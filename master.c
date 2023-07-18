#include "master.h"
#include "app_error.h"
#include "ble.h"
#include "ble_advertising.h"
#include "nrf_ble_scan.h"
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_radio.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrfx_timer.h"
#include "espar_driver.h"
#include "radio.h"
#include "usb_serial.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef ESPAR_GENETIC
#include "espar_genetic.h"
#endif

#define MASTER_WDT_TIMEOUT_MS 100

static bool master_enabled = false;
bool espar_run = false;
#ifdef ESPAR_GENETIC
struct espar_gen_ctx eg_ctx;
#endif
static const nrfx_timer_t master_timer = NRFX_TIMER_INSTANCE(0);
void espar_start()
{
	espar_run = 1;
	espar_set_characteristic(0);
}

void set_char(uint16_t espar_char)
{
	characteristic driver_char = {.passive = {R}};
	for (int i = 0; i < NUMBER_OF_PASSIVE; i++) {
		driver_char.passive[i] = (espar_char >> i) & 1;
	}
	espar_set_custom_characteristic(driver_char);
}

const char *espar_char_as_string(int16_t x)
{
	static char b[13];
	b[0] = '\0';

	int16_t z;
	for (z = 1 << 11; z > 0; z >>= 1) {
		strcat(b, ((x & z) == z) ? "1" : "0");
	}

	return b;
}

uint16_t get_next_char()
{
#ifdef ESPAR_GENETIC
	uint16_t next_char = espar_gen_next_genome_to_eval(&eg_ctx);
	while (next_char == EG_INVALID_GENOME) {
		espar_gen_next_generation(&eg_ctx);
		next_char = espar_gen_next_genome_to_eval(&eg_ctx);
	}
	return next_char;
#else
	static uint16_t next_char = 0;
	return next_char++;
#endif
}

void record_char_rssi(uint16_t espar_char, int8_t rssi)
{
#ifdef ESPAR_GENETIC
	espar_gen_record_fitness(&eg_ctx, espar_char, rssi);
#else
	return;
#endif
}

bool espar_finish() { return false; }

uint32_t last_packet_number = 0;
uint32_t packets_received = 0;
#define BATCH_SIZE 100
uint32_t batch_number = 0;
uint16_t present_espar_char;
uint32_t batch_packets_received = 0;
uint32_t batch_first_packet = 0;

void next_batch()
{
	double bper;
	uint32_t batch_packet_diff;

	master_stop();

	batch_packet_diff = last_packet_number - batch_first_packet + 1;
	if (batch_number) {
		if (!batch_packets_received || !batch_packet_diff)
			bper = 1;
		else {
			bper = (double)(batch_packet_diff -
					batch_packets_received) /
			       batch_packet_diff;
		}
		NRF_LOG_INFO("[BATCH %u SUMMARY]: ESPAR CHAR: %s, BPR: %u, "
			     "BPER: " NRF_LOG_FLOAT_MARKER,
			     batch_number,
			     espar_char_as_string(present_espar_char),
			     batch_packets_received, NRF_LOG_FLOAT(bper));
	}
	batch_number++;
	batch_packets_received = 0;
	batch_first_packet = 0;
	if (present_espar_char >= (1 << 12) - 1) {
		// If last characteristic was reached, finish receiving task
		return;
	} else {
		present_espar_char = get_next_char();
		set_char(present_espar_char);
		master_start();
	}
}

void master_handler(void) {}

static void master_timer_handler(nrf_timer_event_t event_type, void *context)
{
	switch (event_type) {
	case NRF_TIMER_EVENT_COMPARE0:
		/* WDT for receiving a packet, move to next batch if reached */
		next_batch();
		break;
	default:
		break;
	}
}

static void master_timer_init()
{
	nrfx_err_t err;
	nrfx_timer_config_t timer_cfg = {
	    .frequency = NRF_TIMER_FREQ_1MHz,
	    .mode = NRF_TIMER_MODE_TIMER,
	    .bit_width = NRF_TIMER_BIT_WIDTH_24,
	    .p_context = NULL,
	};

	err = nrfx_timer_init(&master_timer, &timer_cfg, master_timer_handler);
	if (err != NRFX_SUCCESS) {
		NRF_LOG_INFO("nrfx_timer_init failed with: %d\n", err);
	}
}

void master_packet_handler(struct radio_packet_t received)
{
	/* kick the watchdog timer as a packet was received */
	nrfx_timer_clear(&master_timer);

	/* discard the value of jammer packets */
	if (!received.data || received.data == 0xffffffff) {
		return;
	}

	packets_received++;

	if (received.data < last_packet_number) {
		/* if received smaller value than last time, then asume new
		 * package set is started */
		packets_received = 1;
		batch_packets_received = 1;
		batch_first_packet = 0;
	}
	last_packet_number = received.data;

	/* brute force  */
	if (!batch_first_packet)
		batch_first_packet = last_packet_number;
	batch_packets_received++;
	if (last_packet_number - batch_first_packet + 1 >= BATCH_SIZE) {
		/* first packet from a new batch received */
		// batch_number = last_packet_number / BATCH_SIZE;
		next_batch();
	}
	/* brute force end */
	

	// NRF_LOG_INFO(
	//     "RSSI: %d, PR: %u, LP: %u,  PER: " NRF_LOG_FLOAT_MARKER,
	//     received.rssi * (-1), packets_received, last_packet_number,
	//     NRF_LOG_FLOAT((double)(last_packet_number - packets_received) /
	// 		  last_packet_number));
}

void master_start()
{
	last_packet_number = 0;
	packets_received = 0;
	master_enabled = true;
	radio_rx(master_packet_handler);
	nrfx_timer_enable(&master_timer);
}

void master_stop()
{
	radio_disable();
	nrfx_timer_disable(&master_timer);
	master_enabled = false;
}

void master_init()
{
	radio_init();
	master_timer_init();
	nrfx_timer_extended_compare(
	    &master_timer, NRF_TIMER_CC_CHANNEL0,
	    nrfx_timer_ms_to_ticks(&master_timer, MASTER_WDT_TIMEOUT_MS),
	    (NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK), true);
	nrf_radio_int_enable(NRF_RADIO_INT_CRCOK_MASK);
	NVIC_EnableIRQ(RADIO_IRQn);
#ifdef BOARD_DD
	espar_init();
	NRF_LOG_INFO("ESPAR INIT DONE");
	espar_start();
	present_espar_char = 0;
	NRF_LOG_INFO("ESPAR START DONE");
#ifdef ESPAR_GENETIC
	espar_gen_init(&eg_ctx);
	NRF_LOG_INFO("ESPAR GENETIC INIT DONE");
#endif
#endif
}
