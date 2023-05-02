#include "client.h"

#include "app_button.h"
#include "app_error.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_lbs.h"
#include "ble_srv_common.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include <stdint.h>
#include <string.h>

#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrfx_timer.h"
#include "radio.h"
#include "usb_serial.h"

static bool client_enabled = false;
static uint32_t packet;

void client_init(void)
{
	radio_init();
	//
}

void client_start(void)
{
	packet = 1;
	client_enabled = true;
	//
}
void client_stop(void) { client_enabled = false; }

void client_handler()
{
	if (!client_enabled)
		return;
	send_packet(packet);
	NRF_LOG_INFO("The contents of the package was %u",
		     (unsigned int)packet);
	packet++;
	NRFX_DELAY_US(10000);
	NRF_LOG_FLUSH();
	// __WFE();
}
