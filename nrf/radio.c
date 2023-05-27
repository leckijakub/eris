#include "app_error.h"
#include "nrf_error.h"
#include "radio_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define RADIO_MAX_PAYLOAD_LEN 256 /**< Maximum radio RX or TX payload. */
#define IEEE_MAX_PAYLOAD_LEN 127  /**< IEEE 802.15.4 maximum payload length. */
#define IEEE_MIN_CHANNEL 11	  /**< IEEE 802.15.4 minimum channel. */
#define IEEE_MAX_CHANNEL 26	  /**< IEEE 802.15.4 maximum channel. */
#define IEEE_DEFAULT_FREQ (5)	  /**< IEEE 802.15.4 default frequency. */
#define IEEE_FREQ_CALC(_channel)                                               \
	(IEEE_DEFAULT_FREQ +                                                   \
	 (IEEE_DEFAULT_FREQ *                                                  \
	  ((_channel)-IEEE_MIN_CHANNEL))) /**< Frequency calculation for a     \
					     given channel in the IEEE         \
					     802.15.4 radio mode. */

static uint32_t packet; /**< Packet to transmit. */
static bool radio_configured = false;

/**@brief Function for disabling radio.
 */
void radio_disable(void)
{
	NRF_RADIO->SHORTS = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
#ifdef NRF51
	NRF_RADIO->TEST = 0;
#endif
	NRF_RADIO->TASKS_DISABLE = 1;

	while (NRF_RADIO->EVENTS_DISABLED == 0) {
		// Do nothing.
	}
	NRF_RADIO->EVENTS_DISABLED = 0;
}

/**@brief Function for reading packet.
 */
uint32_t read_packet()
{
	uint32_t result = 0;

	radio_disable();
	NRF_RADIO->EVENTS_READY = 0U;
	// Enable radio and wait for ready
	NRF_RADIO->TASKS_RXEN = 1U;

	while (NRF_RADIO->EVENTS_READY == 0U) {
		// wait
	}
	NRF_RADIO->EVENTS_END = 0U;
	// Start listening and wait for address received event
	NRF_RADIO->TASKS_START = 1U;

	// Wait for end of packet or buttons state changed
	while (NRF_RADIO->EVENTS_END == 0U) {
		// wait
	}

	if (NRF_RADIO->CRCSTATUS == 1U) {
		result = *((uint32_t *)NRF_RADIO->PACKETPTR);
		// result = packet;
	}
	radio_disable();
	/* NRF_RADIO->EVENTS_DISABLED = 0U;
	// Disable radio
	NRF_RADIO->TASKS_DISABLE = 1U;

	while (NRF_RADIO->EVENTS_DISABLED == 0U) {
		// wait
	} */
	return result;
}

void send_packet(uint32_t packet_to_send)
{
	radio_disable();
	packet = packet_to_send;
	// send the packet:
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

	//     uint32_t err_code = bsp_indication_set(BSP_INDICATE_SENT_OK);
	// NRF_LOG_INFO("The packet was sent");
	//     APP_ERROR_CHECK(err_code);

	radio_disable();
	/* NRF_RADIO->EVENTS_DISABLED = 0U;
	// Disable radio
	NRF_RADIO->TASKS_DISABLE = 1U;

	while (NRF_RADIO->EVENTS_DISABLED == 0U) {
		// wait
	} */
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
	if (power == RADIO_TXPOWER_TXPOWER_0dBm     ||
	    power == RADIO_TXPOWER_TXPOWER_Pos2dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Pos3dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Pos4dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Pos5dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Pos6dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Pos7dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Pos8dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Neg40dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg30dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg20dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg16dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg12dBm ||
	    power == RADIO_TXPOWER_TXPOWER_Neg8dBm  ||
	    power == RADIO_TXPOWER_TXPOWER_Neg4dBm)
	    return true;
	return false;
}

void radio_unmodulated_tx_carrier(uint8_t txpower, uint8_t channel)
{
	radio_disable();
	NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk;
	NRF_RADIO->TXPOWER = (txpower << RADIO_TXPOWER_TXPOWER_Pos);
	// NRF_RADIO->MODE = (mode << RADIO_MODE_MODE_Pos);

	radio_channel_set(channel);
	NRF_RADIO->TASKS_TXEN = 1;
}

void radio_init()
{
	if (radio_configured)
		return;
	// Set radio configuration parameters
	radio_configure();
	NRF_RADIO->PACKETPTR = (uint32_t)&packet;
	NRF_RADIO->PCNF1 =
	    (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
	    (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) |
	    (PACKET_BASE_ADDRESS_LENGTH << RADIO_PCNF1_BALEN_Pos) |
	    (4 << RADIO_PCNF1_STATLEN_Pos) | (4 << RADIO_PCNF1_MAXLEN_Pos);

	NRF_RADIO->FREQUENCY = 2UL; // 2402 MHz - Channel 37
	NRF_LOG_INFO("RADIO INIT DONE");
	radio_configured = true;
}
