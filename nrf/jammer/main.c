#include "ant_interface.h"
#include "ant_parameters.h"
#include "app_error.h"
#include "boards.h"
#include "hardfault.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"
#include "nrf_soc.h"
#include "sdk_config.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_delay.h"
#include "usb_serial.h"
#include "client.h"
#include "master.h"

#define TX_LED BSP_BOARD_LED_2
#define JAMMER_LED BSP_BOARD_LED_3
#define RX_LED BSP_BOARD_LED_0

enum dut_state { DUT_STATE_JAMMING, DUT_STATE_TX, DUT_STATE_RX, DUT_STATE_IDLE };
enum dut_state present_state = DUT_STATE_IDLE;

/**@brief Initialize application.
 */
int jammer_start(uint8_t power_level)
{
	if (power_level < 0 || power_level > 5) {
		USB_SER_PRINT("Invalid power level passed.\r\n");
		return 0;
	}
	uint32_t err_code = sd_ant_cw_test_mode_init();
	APP_ERROR_CHECK(err_code);

	// sd_ant_cw_test_mode assumes that the hfclck is enabled.
	// Request and wait for it to be ready.
	err_code = sd_clock_hfclk_request();
	APP_ERROR_CHECK(err_code);
	nrf_delay_ms(2);
	uint32_t hfclk_is_running = 0;

	while (!hfclk_is_running) {
		APP_ERROR_CHECK(sd_clock_hfclk_is_running(&hfclk_is_running));
	}

	err_code = sd_ant_cw_test_mode(02, power_level, 0, 1);
	APP_ERROR_CHECK(err_code);

	bsp_board_led_on(JAMMER_LED);
	USB_SER_PRINT("Jamming started, Power: %d\r\n", power_level);

	return 1;
}

int jammer_stop()
{
	if (sd_ant_stack_reset() == NRF_ERROR_TIMEOUT) {
		USB_SER_PRINT("Failed to stop jamming\r\n");
		return 0;
	}
	bsp_board_led_off(JAMMER_LED);
	USB_SER_PRINT("Jamming stopped\r\n");
	return 0;
}

int tx_start()
{
	client_start();
	USB_SER_PRINT("TX started\r\n");
	bsp_board_led_on(TX_LED);
	return 0;
}

int tx_stop()
{
	client_stop();
	USB_SER_PRINT("TX stopped\r\n");
	bsp_board_led_off(TX_LED);
	return 0;
}


int rx_start()
{
	scan_start();
	USB_SER_PRINT("RX started\r\n");
	bsp_board_led_on(RX_LED);
	return 0;
}

int rx_stop()
{
	scan_stop();
	USB_SER_PRINT("RX stopped\r\n");
	bsp_board_led_off(RX_LED);
	return 0;
}



/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next
 * event occurs.
 */
static void idle_state_handle(void)
{
	if (usb_ser_events_process() == false) {
		nrf_pwr_mgmt_run();
	}
}

static void leds_init(void) { bsp_board_init(BSP_INIT_LEDS); }

static inline void reset_to_bootloader() { nrf_gpio_cfg_output(19); }

enum espar_cmd {
	ESPAR_CMD_HELP,
	ESPAR_CMD_JAM,
	ESPAR_CMD_IDLE,
	ESPAR_CMD_RESET,
	ESPAR_CMD_UNKNOWN,
	ESPAR_CMD_TX,
	ESPAR_CMD_RX,
};

void print_help()
{
	USB_SER_PRINT("\r\nESPAR DUT device\r\n"
		      "Commands:\r\n\n"
		      "help	- Print this help\r\n"
		      "jam	- Start jamming\r\n"
		      "tx	- Start transmitting\r\n"
		      "idle	- Idle state\r\n"
		      "reset	- Reset the board to bootloader\r\n");
}

enum espar_cmd parse_cmd(char *cmd_buf, size_t cmd_size)
{
	// USB_SER_PRINT("PARSING CMD: %s", cmd_buf);
	if (!strncmp("help", cmd_buf, cmd_size)) {
		return ESPAR_CMD_HELP;
	} else if (!strncmp("jam", cmd_buf, cmd_size)) {
		return ESPAR_CMD_JAM;
	} else if (!strncmp("tx", cmd_buf, cmd_size)) {
		return ESPAR_CMD_TX;
	} else if (!strncmp("idle", cmd_buf, cmd_size)) {
		return ESPAR_CMD_IDLE;
	} else if (!strncmp("reset", cmd_buf, cmd_size)) {
		return ESPAR_CMD_RESET;
	} else if (!strncmp("rx", cmd_buf, cmd_size)) {
		return ESPAR_CMD_RX;
	}

	return ESPAR_CMD_UNKNOWN;
}

int dut_set_state(enum dut_state state, int power_level)
{
	if (present_state == state) {
		USB_SER_PRINT("DUT already in requested state\r\n");
		return 1;
	}

	/* reset state to idle if other state was active  */
	if (present_state != DUT_STATE_IDLE) {
		switch (present_state) {
		case DUT_STATE_JAMMING:
			jammer_stop();
			break;
		case DUT_STATE_TX:
			tx_stop();
			break;
		case DUT_STATE_RX:
			rx_stop();
			break;
		default:
			break;
		}
		present_state = DUT_STATE_IDLE;
	}

	/* set requested state */
	switch (state) {
	case DUT_STATE_JAMMING:
		if (!jammer_start(power_level)) {
			return 0;
		}
		break;
	case DUT_STATE_TX:
		tx_start();
		break;
	case DUT_STATE_RX:
		rx_start();
		break;
	default:
		break;
	}

	present_state = state;
	return 1;
}

void input_handler(char *input_buff, size_t input_size)
{
	// struct espar_ctrl ctrl;
	static char cmd_buff[128] = {0};
	static char cmd_buf_size = 0;
	char cmd[128];
	int arg;

	// echo input back to console
	USB_SER_PRINT("%.*s", input_size, input_buff);
	if(cmd_buf_size + input_size >= 128){
		USB_SER_PRINT("\r\nCMD buffer exceeded\r\n");
		goto cmd_exit;
	}
	// store input in buffer for later parsing
	memcpy(cmd_buff + cmd_buf_size, input_buff, input_size);
	cmd_buf_size += input_size;

	// parse command if enter was pressed
	if (cmd_buff[cmd_buf_size - 1] == '\r') {

		// enable printing to serial only after first enter
		usb_ser_enable();
		USB_SER_PRINT("\n");

		// separate command from its arguments
		cmd_buff[cmd_buf_size - 1] = '\0';
		cmd[0] = '\0';
		arg = -1;
		sscanf(cmd_buff, "%s %d", cmd, &arg);

		if (strlen(cmd) == 0) {
			goto cmd_exit;
		}

		// handle commands
		switch (parse_cmd(cmd, strlen(cmd))) {
		case ESPAR_CMD_HELP:
			print_help();
			break;
		case ESPAR_CMD_JAM:
			dut_set_state(DUT_STATE_JAMMING, arg);
			break;
		case ESPAR_CMD_TX:
			dut_set_state(DUT_STATE_TX, arg);
			break;
		case ESPAR_CMD_RX:
			dut_set_state(DUT_STATE_RX, arg);
			break;
		case ESPAR_CMD_IDLE:
			dut_set_state(DUT_STATE_IDLE, arg);
			break;
		case ESPAR_CMD_RESET:
			USB_SER_PRINT("Resetting to bootloader\r\n");
			reset_to_bootloader();
			break;
		case ESPAR_CMD_UNKNOWN:
			USB_SER_PRINT("Unknown command: \"%s\" \r\n", cmd_buff);
		default:
			break;
		}
	cmd_exit:
		cmd_buf_size = 0;
		USB_SER_PRINT("$ ");
	}
}

/* Main function */
int main(void)
{
	ret_code_t err_code;

	leds_init();
	usb_ser_init(&input_handler);
	USB_SER_PRINT("[INFO]: Log init done.\r\n");
	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
	client_init();
	master_init();

	USB_SER_PRINT("ESPAR DUT device started.\r\n");

	// bsp_board_led_on(BSP_BOARD_LED_0);

	// Enter main loop.
	for (;;) {
		idle_state_handle();
	}
}
