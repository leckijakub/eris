#include "app_error.h"
#include "boards.h"
#include "hardfault.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_soc.h"
#include "sdk_config.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_timer.h"
#include "client.h"
#include "fds.h"
#include "jammer.h"
#include "master.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rng.h"
#include "radio.h"
#include "usb_serial.h"

#ifndef BOARD_PCA10059
#include "espar_driver.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#endif
#define TX_LED BSP_BOARD_LED_2
#define JAMMER_LED BSP_BOARD_LED_3
#define RX_LED BSP_BOARD_LED_0

enum dut_state {
	DUT_STATE_JAMMING,
	DUT_STATE_TX,
	DUT_STATE_RX,
	DUT_STATE_IDLE
};
enum dut_state present_state = DUT_STATE_IDLE;

struct dut_config_t {
	enum dut_state state;
	uint8_t power;
};

struct dut_config_t present_config = {
    .state = DUT_STATE_IDLE,
    .power = 0,
};

void storage_update();

int jam_start(uint8_t power_level)
{
	if (!jammer_start(power_level)) {
		return 0;
	}
	bsp_board_led_on(JAMMER_LED);
	USB_SER_PRINT("Jamming started, Power: %d\r\n", power_level);
	return 1;
}

int jam_stop()
{
	jammer_stop();
	bsp_board_led_off(JAMMER_LED);
	USB_SER_PRINT("Jamming stopped\r\n");
	return 1;
}

int tx_start(uint8_t power_level)
{
	if (!client_start(power_level)) {
		return 0;
	}
	USB_SER_PRINT("TX started\r\n");
	bsp_board_led_on(TX_LED);
	return 1;
}

int tx_stop()
{
	client_stop();
	USB_SER_PRINT("TX stopped\r\n");
	bsp_board_led_off(TX_LED);
	return 1;
}

int rx_start()
{
	// scan_start();
	master_start();
	USB_SER_PRINT("RX started\r\n");
	bsp_board_led_on(RX_LED);
	return 1;
}

int rx_stop()
{
	// scan_stop();
	master_stop();
	USB_SER_PRINT("RX stopped\r\n");
	bsp_board_led_off(RX_LED);
	return 1;
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
			jam_stop();
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
		if (!jam_start(power_level)) {
			return 0;
		}
		break;
	case DUT_STATE_TX:
		if (!tx_start(power_level)) {
			return 0;
		}
		break;
	case DUT_STATE_RX:
		rx_start();
		break;
	default:
		break;
	}

	present_state = state;
	present_config.state = state;
	present_config.power = power_level;
#ifndef BOARD_DD
	storage_update();
#endif
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
	if (cmd_buf_size + input_size >= 128) {
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

static void log_init(void)
{
	ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initialization oscillators.
 */
void clock_initialization()
{
	/* Start 16 MHz crystal oscillator */
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	/* Wait for the external oscillator to start up */
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
		// Do nothing.
	}

	/* Start low frequency crystal oscillator for app_timer(used by bsp)*/
	NRF_CLOCK->LFCLKSRC =
	    (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;

	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {
		// Do nothing.
	}
}

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;
fds_record_desc_t desc = {0};
fds_find_token_t tok = {0};
#define CONFIG_FILE (0x8010)
#define CONFIG_REC_KEY (0x7010)
static fds_record_t const fds_config_record = {
    .file_id = CONFIG_FILE,
    .key = CONFIG_REC_KEY,
    .data.p_data = &present_config,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(present_config) + 3) / sizeof(uint32_t),
};

/* Array to map FDS events to strings. */
static char const *fds_evt_str[] = {
    "FDS_EVT_INIT",	  "FDS_EVT_WRITE",    "FDS_EVT_UPDATE",
    "FDS_EVT_DEL_RECORD", "FDS_EVT_DEL_FILE", "FDS_EVT_GC",
};
/* Keep track of the progress of a delete_all operation. */
static struct {
	bool delete_next; //!< Delete next record.
	bool pending;	  //!< Waiting for an fds FDS_EVT_DEL_RECORD event, to
			  //!< delete the next record.
} m_delete_all;

const char *fds_err_str(ret_code_t ret)
{
	/* Array to map FDS return values to strings. */
	static char const *err_str[] = {
	    "FDS_ERR_OPERATION_TIMEOUT", "FDS_ERR_NOT_INITIALIZED",
	    "FDS_ERR_UNALIGNED_ADDR",	 "FDS_ERR_INVALID_ARG",
	    "FDS_ERR_NULL_ARG",		 "FDS_ERR_NO_OPEN_RECORDS",
	    "FDS_ERR_NO_SPACE_IN_FLASH", "FDS_ERR_NO_SPACE_IN_QUEUES",
	    "FDS_ERR_RECORD_TOO_LARGE",	 "FDS_ERR_NOT_FOUND",
	    "FDS_ERR_NO_PAGES",		 "FDS_ERR_USER_LIMIT_REACHED",
	    "FDS_ERR_CRC_CHECK_FAILED",	 "FDS_ERR_BUSY",
	    "FDS_ERR_INTERNAL",
	};

	return err_str[ret - NRF_ERROR_FDS_ERR_BASE];
}
static void fds_evt_handler(fds_evt_t const *p_evt)
{
	if (p_evt->result == NRF_SUCCESS) {
		NRF_LOG_INFO("Event: %s received (NRF_SUCCESS)",
			     fds_evt_str[p_evt->id]);
	} else {
		NRF_LOG_INFO("Event: %s received (%s)", fds_evt_str[p_evt->id],
			     fds_err_str(p_evt->result));
	}

	switch (p_evt->id) {
	case FDS_EVT_INIT:
		if (p_evt->result == NRF_SUCCESS) {
			m_fds_initialized = true;
		}
		break;

	case FDS_EVT_WRITE: {
		if (p_evt->result == NRF_SUCCESS) {
			NRF_LOG_INFO("Record ID:\t0x%04x",
				     p_evt->write.record_id);
			NRF_LOG_INFO("File ID:\t0x%04x", p_evt->write.file_id);
			NRF_LOG_INFO("Record key:\t0x%04x",
				     p_evt->write.record_key);
		}
	} break;

	case FDS_EVT_DEL_RECORD: {
		if (p_evt->result == NRF_SUCCESS) {
			NRF_LOG_INFO("Record ID:\t0x%04x",
				     p_evt->del.record_id);
			NRF_LOG_INFO("File ID:\t0x%04x", p_evt->del.file_id);
			NRF_LOG_INFO("Record key:\t0x%04x",
				     p_evt->del.record_key);
		}
		m_delete_all.pending = false;
	} break;

	default:
		break;
	}
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
	while (!m_fds_initialized) {
	}
}

void storage_init()
{
	uint32_t err_code = NRF_SUCCESS;

	/* Register first to receive an event when initialization is complete.
	 */
	(void)fds_register(fds_evt_handler);

	err_code = fds_init();
	APP_ERROR_CHECK(err_code);

	/* Wait for fds to initialize. */
	wait_for_fds_ready();
	fds_stat_t stat = {0};

	err_code = fds_stat(&stat);
	APP_ERROR_CHECK(err_code);

	err_code = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);
	if (err_code != NRF_SUCCESS) {
		// FDS record wasn't found, create it
		/* System config not found; write a new one. */
		NRF_LOG_INFO("Writing config file...");
		USB_SER_PRINT("Writing config file...\r\n");

		err_code = fds_record_write(&desc, &fds_config_record);
		if ((err_code != NRF_SUCCESS) &&
		    (err_code == FDS_ERR_NO_SPACE_IN_FLASH)) {
			NRF_LOG_INFO("No space in flash, delete some records "
				     "to update the config file.");
			USB_SER_PRINT("No space in flash, delete some records "
				      "to update the config file.\r\n");
		} else {
			APP_ERROR_CHECK(err_code);
		}
	}
}

void storage_load()
{
	uint32_t err_code = NRF_SUCCESS;
	/* A config file is in flash. Let's update it. */
	fds_flash_record_t config_flash_record = {0};

	/* Open the record and read its contents. */
	err_code = fds_record_open(&desc, &config_flash_record);
	APP_ERROR_CHECK(err_code);

	/* Copy the configuration from flash into m_dummy_cfg. */
	memcpy(&present_config, config_flash_record.p_data,
	       sizeof(struct dut_config_t));

	/* Close the record when done reading. */
	err_code = fds_record_close(&desc);
	APP_ERROR_CHECK(err_code);

	USB_SER_PRINT("Present config: state:%d, power=%d\r\n",
		      present_config.state, present_config.power);
}

void storage_update()
{
	uint32_t err_code = NRF_SUCCESS;
	/* Write the updated record to flash. */
	err_code = fds_record_update(&desc, &fds_config_record);
	if ((err_code != NRF_SUCCESS) &&
	    (err_code == FDS_ERR_NO_SPACE_IN_FLASH)) {
		NRF_LOG_INFO("No space in flash, delete some records to update "
			     "the config file.");
	} else {
		APP_ERROR_CHECK(err_code);
	}
}

/**
 * @brief Function for application main entry.
 * @return 0. int return type required by ANSI/ISO standard.
 */
int main(void)
{
	uint32_t err_code = NRF_SUCCESS;

	clock_initialization();
	err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);

	log_init();
	leds_init();
	usb_ser_init(&input_handler);
	NRF_LOG_INFO("RTT LOG INIT DONE");
	USB_SER_PRINT("[INFO]: Log init done.\r\n");
	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);

	master_init();
	NRF_LOG_INFO("MASTER INIT DONE");
	client_init();
	NRF_LOG_INFO("CLIENT INIT DONE");
	jammer_init();
	NRF_LOG_INFO("JAMMER INIT DONE");
#ifdef BOARD_DD
	master_start();
#else
	storage_init();
	storage_load();
	dut_set_state(present_config.state, present_config.power);
	// jam_start(0);
#endif
	NRF_LOG_FLUSH();
	while (true) {
		UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
		usb_ser_events_process();
		master_handler();
		client_handler();
		jammer_handler();
	}
}
