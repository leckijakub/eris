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

	uint32_t hfclk_is_running = 0;

	while (!hfclk_is_running) {
		APP_ERROR_CHECK(sd_clock_hfclk_is_running(&hfclk_is_running));
	}

	// CW Mode at +4dBm, 2402 MHz with Modulated Transmission
	err_code = sd_ant_cw_test_mode(02, RADIO_TX_POWER_LVL_1, 0, 1);
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

void input_parser(char *buff, size_t size)
{
	buff[size] = '\0';
	USB_SER_PRINT("%s", buff);
	if (buff[size - 1] == '\r') {
		usb_ser_enable();
		USB_SER_PRINT("\n");
	}
}

/* Main function */
int main(void)
{
	leds_init();
	usb_ser_init(&input_parser);
	USB_SER_PRINT("[INFO]: Log init done.\r\n");
	softdevice_setup();
	USB_SER_PRINT("[INFO]: Softdevice init done.\r\n");
	application_initialize();
	USB_SER_PRINT("[INFO]: Application init done.\r\n");

	USB_SER_PRINT("ANT Continuous Waveform Mode example started.\r\n");
	// Enter main loop.
	for (;;) {
		idle_state_handle();
	}
}
