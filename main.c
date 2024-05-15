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

#include "nrf_cli.h"
#include "nrf_cli_rtt.h"
#include "nrf_cli_types.h"

#define TX_LED BSP_BOARD_LED_2
#define JAMMER_LED BSP_BOARD_LED_3
#define RX_LED BSP_BOARD_LED_0

#define CLI_EXAMPLE_LOG_QUEUE_SIZE  (4)

NRF_CLI_RTT_DEF(m_cli_rtt_transport);
NRF_CLI_DEF(m_cli_rtt,
#ifdef BOARD_DD
            "espar_cli:~$ ",
#else
			"beacon_cli:~$ ",
#endif
            &m_cli_rtt_transport.transport,
            '\n',
            CLI_EXAMPLE_LOG_QUEUE_SIZE);

static void cli_start(void)
{
    ret_code_t ret;

    ret = nrf_cli_start(&m_cli_rtt);
    APP_ERROR_CHECK(ret);
}

static void cli_init(void)
{
    ret_code_t ret;

    ret = nrf_cli_init(&m_cli_rtt, NULL, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);
}

static void cli_process(void)
{
    nrf_cli_process(&m_cli_rtt);
}

static void leds_init(void)
{
	bsp_board_init(BSP_INIT_LEDS);
}

static void log_init(void)
{
	ret_code_t err_code = NRF_LOG_INIT(app_timer_cnt_get);
	APP_ERROR_CHECK(err_code);

	// Backends are initialized by cli
	// NRF_LOG_DEFAULT_BACKENDS_INIT();
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
	// usb_ser_init(&input_handler);
	cli_init();
	NRF_LOG_INFO("RTT LOG INIT DONE");
	USB_SER_PRINT("[INFO]: Log init done.\r\n");
	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
	err_code = nrf_drv_rng_init(NULL);
	APP_ERROR_CHECK(err_code);

	master_init();
	NRF_LOG_INFO("MASTER INIT DONE");
	client_init();
	NRF_LOG_INFO("CLIENT INIT DONE");
	jammer_init();
	NRF_LOG_INFO("JAMMER INIT DONE");

	cli_start();
	NRF_LOG_RAW_INFO("Command Line Interface started.\n");
    NRF_LOG_RAW_INFO("Please press the Tab key to see all available commands.\n");

// #ifdef BOARD_DD
// 	master_start();
// #endif
	NRF_LOG_FLUSH();
	while (true) {
		UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
		cli_process();
		usb_ser_events_process();
		master_handler();
		client_handler();
		jammer_handler();
	}
}
