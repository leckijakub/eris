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

/**@brief Initialize application.
 */
static void application_initialize()
{
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

	// CW Mode at +4dBm, 2402 MHz with Modulated Transmission
	err_code = sd_ant_cw_test_mode(02, RADIO_TX_POWER_LVL_5, 0, 1);
	// err_code = sd_ant_cw_test_mode(26, RADIO_TX_POWER_LVL_5, 0, 1);
	// err_code = sd_ant_cw_test_mode(80, RADIO_TX_POWER_LVL_5, 0, 1);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for ANT stack initialization.
 *
 * @details Initializes the SoftDevice and the ANT event interrupt.
 */
static void softdevice_setup(void)
{
	ret_code_t err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	ASSERT(nrf_sdh_is_enabled());

	// err_code = sd_ant_enable();
	// APP_ERROR_CHECK(err_code);
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
	ESPAR_CMD_START,
	ESPAR_CMD_STOP,
	ESPAR_CMD_RESET,
	ESPAR_CMD_UNKNOWN,
};

void print_help()
{
	USB_SER_PRINT("\r\nESPAR DUT device\r\n"
		      "Commands:\r\n\n"
		      "help	- Print this help\r\n"
		      "start	- Start jamming\r\n"
		      "stop	- Stop jamming\r\n"
		      "reset	- reset the board to bootloader - WARNING: Application will be erased!!!\r\n");
}

enum espar_cmd parse_cmd(char *cmd_buf, size_t cmd_size)
{
	// USB_SER_PRINT("PARSING CMD: %s", cmd_buf);
	if (!strncmp("help", cmd_buf, cmd_size)) {
		return ESPAR_CMD_HELP;
	} else if (!strncmp("start", cmd_buf, cmd_size)) {
		return ESPAR_CMD_START;
	} else if (!strncmp("stop", cmd_buf, cmd_size)) {
		return ESPAR_CMD_STOP;
	} else if (!strncmp("reset", cmd_buf, cmd_size)) {
		return ESPAR_CMD_RESET;
	}

	return ESPAR_CMD_UNKNOWN;
}

void input_handler(char *input_buff, size_t input_size)
{
	// struct espar_ctrl ctrl;
	static char cmd_buff[64] = {0};
	static char cmd_buf_size = 0;
	char cmd[32];
	int arg;

	// echo input back to console
	USB_SER_PRINT("%.*s", input_size, input_buff);

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
		case ESPAR_CMD_START:
			if (arg == -1) {
				USB_SER_PRINT(
				    "Specify power level to start jamming\r\n");
				goto cmd_exit;
			}
			USB_SER_PRINT("Jamming START, Power: %d\r\n", arg);
			bsp_board_led_on(BSP_BOARD_LED_3);
			break;
		case ESPAR_CMD_STOP:
			USB_SER_PRINT("Jamming STOP\r\n");
			bsp_board_led_off(BSP_BOARD_LED_3);
			break;
		case ESPAR_CMD_RESET:
			USB_SER_PRINT("Resetting to bootloader\r\n");
			reset_to_bootloader();
			break;
		case ESPAR_CMD_UNKNOWN:
			USB_SER_PRINT("Unknown command\r\n");
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
	leds_init();
	usb_ser_init(&input_handler);
	USB_SER_PRINT("[INFO]: Log init done.\r\n");
	softdevice_setup();
	USB_SER_PRINT("[INFO]: Softdevice init done.\r\n");
	application_initialize();
	USB_SER_PRINT("[INFO]: Application init done.\r\n");

	USB_SER_PRINT("ANT Continuous Waveform Mode example started.\r\n");

	bsp_board_led_on(BSP_BOARD_LED_0);

	// Enter main loop.
	for (;;) {
		idle_state_handle();
	}
}
