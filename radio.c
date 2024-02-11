#include "radio.h"
#include "app_error.h"
#include "nrf_error.h"
#include "nrf_radio.h"
#include "radio_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define RADIO_MAX_PAYLOAD_LEN 256 /**< Maximum radio RX or TX payload. */
#define IEEE_MAX_PAYLOAD_LEN 127 /**< IEEE 802.15.4 maximum payload length. */
#define IEEE_MIN_CHANNEL 11 /**< IEEE 802.15.4 minimum channel. */
#define IEEE_MAX_CHANNEL 26 /**< IEEE 802.15.4 maximum channel. */
#define IEEE_DEFAULT_FREQ (5) /**< IEEE 802.15.4 default frequency. */
#define IEEE_FREQ_CALC(_channel) \
	(IEEE_DEFAULT_FREQ +     \
	 (IEEE_DEFAULT_FREQ *    \
	  ((_channel)-IEEE_MIN_CHANNEL))) /**< Frequency calculation for a     \
					     given channel in the IEEE         \
					     802.15.4 radio mode. */

enum radio_state { RADIO_RECEIVING, RADIO_TRANSMITTING, RADIO_IDLE };

struct radio_config_t {
	void (*rx_handler)(struct radio_packet_t);
	void (*tx_handler)(struct radio_packet_t *);
	enum radio_state state;
};
static volatile struct radio_config_t radio_config = { 0 };

static uint32_t packet = 0xffffffff; /**< Packet to transmit. */
static bool radio_configured = false;

/**@brief Function for disabling radio.
 */
void radio_disable(void)
{
	nrf_radio_shorts_set(0);
	nrf_radio_int_disable(~0);
	nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);

	nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
	while (!nrf_radio_event_check(NRF_RADIO_EVENT_DISABLED)) {
		/* Do nothing */
	}
	nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
	radio_config.state = RADIO_IDLE;
}

void send_packet(uint32_t packet_to_send)
{
	radio_disable();
	packet = packet_to_send;
	// send the packet:
	radio_config.state = RADIO_TRANSMITTING;
	NRF_RADIO->EVENTS_READY = 0U;
	NRF_RADIO->TASKS_TXEN = 1;

	while (NRF_RADIO->EVENTS_READY == 0U) {
		// wait
	}
	NRF_RADIO->EVENTS_END = 0U;
	NRF_RADIO->TASKS_START = 1U;

	while (NRF_RADIO->EVENTS_END == 0U) {
		// wait
	}
	radio_disable();
}

/**brief Function for setting the channel for radio.
 *
 * @param[in] mode    Radio mode.
 * @param[in] channel Radio channel.
 */
static void radio_channel_set(uint8_t channel)
{
	NRF_RADIO->FREQUENCY = channel;
}

bool radio_power_level_valid(uint8_t power)
{
	if (power == RADIO_TXPOWER_TXPOWER_0dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos2dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos3dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos4dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos5dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos6dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos7dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Pos8dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg40dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg30dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg20dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg16dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg12dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg8dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg4dBm)
		return true;
	return false;
}

void radio_unmodulated_tx_carrier(uint8_t txpower, uint8_t channel)
{
	radio_disable();
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk;
	NRF_RADIO->TXPOWER = (txpower << RADIO_TXPOWER_TXPOWER_Pos);

	radio_channel_set(channel);
	NRF_RADIO->TASKS_TXEN = 1;
}

static void radio_start(bool rx)
{
	radio_config.state = rx ? RADIO_RECEIVING : RADIO_TRANSMITTING;
	nrf_radio_task_trigger(rx ? NRF_RADIO_TASK_RXEN : NRF_RADIO_TASK_TXEN);
}

void radio_rx(void (*packet_handler)(struct radio_packet_t))
{
	radio_disable();

	nrf_radio_shorts_enable(NRF_RADIO_SHORT_READY_START_MASK |
				NRF_RADIO_SHORT_END_START_MASK |
				NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK);
	nrf_radio_packetptr_set(&packet);

	radio_config.rx_handler = packet_handler;
	nrf_radio_int_enable(NRF_RADIO_INT_CRCOK_MASK);
	radio_start(true);
}

void radio_tx(void (*packet_handler)(struct radio_packet_t *))
{
	radio_disable();
	packet = 0;

	nrf_radio_shorts_enable(NRF_RADIO_SHORT_READY_START_MASK |
				NRF_RADIO_SHORT_END_START_MASK);

	radio_config.tx_handler = packet_handler;
	nrf_radio_event_clear(NRF_RADIO_EVENT_END);
	nrf_radio_int_enable(NRF_RADIO_INT_END_MASK);

	radio_start(false);
}

void radio_init()
{
	if (radio_configured)
		return;
	// Set radio configuration parameters
	radio_configure();
	NVIC_EnableIRQ(RADIO_IRQn);
	NRF_RADIO->PACKETPTR = (uint32_t)&packet;
	NRF_RADIO->PCNF1 =
		(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
		(RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) |
		(PACKET_BASE_ADDRESS_LENGTH << RADIO_PCNF1_BALEN_Pos) |
		(4 << RADIO_PCNF1_STATLEN_Pos) | (4 << RADIO_PCNF1_MAXLEN_Pos);

	NRF_RADIO->FREQUENCY = 80UL; // 2480 MHz - Channel 39
	NRF_LOG_INFO("RADIO INIT DONE");
	radio_configured = true;
}

void RADIO_IRQHandler(void)
{
	struct radio_packet_t tx_packet;
	struct radio_packet_t rx_packet;
	if (nrf_radio_event_check(NRF_RADIO_EVENT_CRCOK)) {
		if (radio_config.state == RADIO_RECEIVING) {
			nrf_radio_event_clear(NRF_RADIO_EVENT_CRCOK);
			rx_packet.data =
				*((uint32_t *)nrf_radio_packetptr_get());
			rx_packet.rssi = nrf_radio_rssi_sample_get();
			radio_config.rx_handler(rx_packet);
		}
	}
	if (nrf_radio_event_check(NRF_RADIO_EVENT_END)) {
		if (radio_config.state == RADIO_TRANSMITTING) {
			nrf_radio_event_clear(NRF_RADIO_EVENT_END);
			tx_packet.data =
				*((uint32_t *)nrf_radio_packetptr_get());
			tx_packet.rssi = nrf_radio_rssi_sample_get();
			radio_config.tx_handler(&tx_packet);
			*(uint32_t *)nrf_radio_packetptr_get() = tx_packet.data;
		}
	}
}
