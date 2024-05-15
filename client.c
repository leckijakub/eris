#include "client.h"

#include "app_error.h"
#include "app_timer.h"
#include "boards.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_pwr_mgmt.h"
#include <stdint.h>
#include <string.h>

#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrfx_timer.h"
#include "radio.h"
// #include "usb_serial.h"

#define CLIENT_PACKET_INTERVAL_MS 5

static bool client_enabled = false;
static uint32_t packet;
static const nrfx_timer_t client_timer = NRFX_TIMER_INSTANCE(1);

void client_packet_handler(struct radio_packet_t *packet)
{
	packet->data++;
}

int client_start(uint8_t power_level)
{
	if (!radio_power_level_valid(power_level)) {
		// USB_SER_PRINT("Invalid power level\r\n");
		NRF_LOG_INFO("Invalid power level");
		return 0;
	}
	NRF_RADIO->TXPOWER = (power_level << RADIO_TXPOWER_TXPOWER_Pos);
	packet = 1;
	client_enabled = true;
	radio_tx(client_packet_handler);
	// nrfx_timer_enable(&client_timer);
	return 1;
}

void client_stop(void)
{
	client_enabled = false;
	nrfx_timer_disable(&client_timer);
	radio_disable();
}

static void client_timer_handler(nrf_timer_event_t event_type, void *context)
{
	switch (event_type) {
	case NRF_TIMER_EVENT_COMPARE0:
		send_packet(packet);
		NRF_LOG_INFO("The contents of the package was %u",
			     (unsigned int)packet);
		packet++;
		break;
	default:
		break;
	}
}

static void client_timer_init()
{
	nrfx_err_t err;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRF_TIMER_FREQ_1MHz,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_24,
		.p_context = NULL,
	};

	err = nrfx_timer_init(&client_timer, &timer_cfg, client_timer_handler);
	if (err != NRFX_SUCCESS) {
		// USB_SER_PRINT("nrfx_timer_init failed with: %ld\r\n", err);
		NRF_LOG_INFO("nrfx_timer_init failed with: %ld", err);
	}
	// USB_SER_PRINT("CLIENT TIMER INITIALIZED\r\n");
	NRF_LOG_INFO("CLIENT TIMER INITIALIZED");
}

void client_handler()
{
	// if (!client_enabled)
	// 	return;
	// send_packet(packet);
	// NRF_LOG_INFO("The contents of the package was %u",
	// 	     (unsigned int)packet);
	// packet++;
	// NRFX_DELAY_US(10000);
	// NRF_LOG_FLUSH();
	// __WFE();
}

void client_init(void)
{
	radio_init();
	client_timer_init();
	nrfx_timer_extended_compare(
		&client_timer, NRF_TIMER_CC_CHANNEL0,
		nrfx_timer_ms_to_ticks(&client_timer,
				       CLIENT_PACKET_INTERVAL_MS),
		(NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK), true);
}
