#include "master.h"
#include "app_error.h"
#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_radio.h"
#include "nrfx_timer.h"
#include "nrf_drv_rng.h"
#include "nrf_delay.h"
#include "espar_driver.h"
#include "radio.h"
#include "usb_serial.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "limits.h"
#include <float.h>

#define MASTER_WDT_TIMEOUT_MS 10

static bool master_enabled = false;
bool espar_run = false;
static const nrfx_timer_t master_timer = NRFX_TIMER_INSTANCE(0);

uint32_t last_packet_number = 0;
uint32_t packets_received = 0;
#define BATCH_SIZE 100
#define BPER_THRESHOLD 0.20
#define CHAR_MAX_COMB 4096 // 2^12 max combinations of passive characteristics
uint32_t batch_number = 0;
uint16_t present_espar_char;
uint32_t batch_packets_received = 0;
uint32_t batch_rssi_sum = 0;
uint32_t batch_first_packet = 0;
uint16_t chars_array[CHAR_MAX_COMB];
uint16_t best_char = 0;
double best_char_bper = DBL_MAX;

void set_char(uint16_t espar_char)
{
	characteristic driver_char = { .passive = { R } };
	for (int i = 0; i < NUMBER_OF_PASSIVE; i++) {
		driver_char.passive[i] = (espar_char >> i) & 1;
	}
	espar_set_custom_characteristic(driver_char);
}

void espar_start()
{
	espar_run = 1;
	// present_espar_char = 0b111111111111;
	present_espar_char = 0;
	set_char(present_espar_char);

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

/** @brief Function for getting vector of random numbers.
 *
 * @param[out] p_buff       Pointer to unit8_t buffer for storing the bytes.
 * @param[in]  size       Number of bytes to take from pool and place in p_buff.
 *
 */
void random_vector_generate(uint8_t *p_buff, uint8_t size)
{
	nrf_drv_rng_block_rand(p_buff, size);
}

uint16_t get_next_char()
{
	static uint16_t next_char = 0;
	next_char = next_char % CHAR_MAX_COMB;
	return chars_array[next_char++];
}

void next_batch()
{
	double bper;
	uint32_t batch_packet_diff;
	double batch_rssi_avg;
	master_stop();

	batch_packet_diff = last_packet_number - batch_first_packet + 1;
	if (batch_number) {
		if (!batch_packets_received || !batch_packet_diff) {
			bper = 1;
			batch_rssi_avg = 0;
		} else {
			bper = (double)(batch_packet_diff -
					batch_packets_received) /
			       batch_packet_diff;
			batch_rssi_avg = -(double)(batch_rssi_sum) /
					 batch_packets_received;
		}
#ifdef BOARD_DD // ESPAR
		char msg[512];
		snprintf(msg, 512,
			 "[BATCH %lu SUMMARY]: ESPAR CHAR: %s, "
			 "BPER: " NRF_LOG_FLOAT_MARKER
			 ", RSSI: " NRF_LOG_FLOAT_MARKER,
			 batch_number, espar_char_as_string(present_espar_char),
			 NRF_LOG_FLOAT(bper), NRF_LOG_FLOAT(batch_rssi_avg));
		NRF_LOG_RAW_INFO("\n%s\n", msg);

		// update best char
		if (bper < best_char_bper) {
			best_char = present_espar_char;
			best_char_bper = bper;
		}
		// update best char bper
		if (present_espar_char == best_char) {
			best_char_bper = bper;
		}
		// use best char if bper lower than 0.20
		/* if (best_char_bper <= BPER_THRESHOLD) {
			present_espar_char = best_char;
		} else {
			present_espar_char = get_next_char();
		} */
		present_espar_char = get_next_char();
		set_char(present_espar_char);
#else // BEACON \
	// print only one of 100 batches as USB serial throughput is limited
		if (batch_number % 100 == 0) {
			USB_SER_PRINT("[BATCH %lu SUMMARY]: BPR: %lu, "
				      "BPER: " NRF_LOG_FLOAT_MARKER
				      ", RSSI: " NRF_LOG_FLOAT_MARKER "\r\n",
				      batch_number, batch_packets_received,
				      NRF_LOG_FLOAT(bper),
				      NRF_LOG_FLOAT(batch_rssi_avg));
		}
#endif
	}

	batch_number++;
	batch_packets_received = 0;
	batch_first_packet = 0;
	batch_rssi_sum = 0;
	master_start();
}

void master_handler(void)
{
}

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
	nrfx_timer_extended_compare(
		&master_timer, NRF_TIMER_CC_CHANNEL0,
		nrfx_timer_ms_to_ticks(&master_timer, MASTER_WDT_TIMEOUT_MS),
		(NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK), true);
}

void master_packet_handler(struct radio_packet_t received)
{
	/* kick the watchdog timer as a packet was received */
	nrfx_timer_clear(&master_timer);

	/* discard the value of jammer packets */
	if (!received.data || received.data == 0xffffffff) {
		return;
	}

	if (received.data < last_packet_number) {
		/* if received smaller value than last time, then asume new
		 * package set is started */
		packets_received = 0;
		batch_packets_received = 0;
		batch_rssi_sum = 0;
		batch_first_packet = 0;
	}

	packets_received++;
	last_packet_number = received.data;
	batch_packets_received++;
	batch_rssi_sum += received.rssi;

	/* batch finalization  */
	if (!batch_first_packet)
		batch_first_packet = last_packet_number;
	if (last_packet_number - batch_first_packet + 1 >= BATCH_SIZE) {
		/* first packet from a new batch received */
		next_batch();
	}

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

void shuffle(uint16_t *array, size_t n)
{
	int rnd;
	if (n > 1) {
		size_t i;
		for (i = 0; i < n - 1; i++) {
			random_vector_generate((uint8_t *)&rnd, sizeof(int));
			// size_t j = i + rnd / (INT_MAX / (n - i) + 1);
			size_t j = i + rnd % (n - i);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}
void init_array_range(uint16_t *array, size_t size, int start, int stop)
{
	if (size != stop - start) {
		NRF_LOG_INFO("INVALID RANGE PASSED");
		return;
	}
	for (int val = start; val < stop; val++) {
		array[val - start] = val;
	}
}

void master_init()
{
	radio_init();
	master_timer_init();
	nrf_radio_int_enable(NRF_RADIO_INT_CRCOK_MASK);
	NVIC_EnableIRQ(RADIO_IRQn);

	// Generate array of all possible passive characteristics from 0 to 2^12
	init_array_range(chars_array, CHAR_MAX_COMB, 0, CHAR_MAX_COMB);
	// Delay to allow log reader to connect
	nrf_delay_ms(5000);
	// randomize the order of characteristics
	// shuffle(chars_array, CHAR_MAX_COMB);

#ifdef BOARD_DD
	espar_init();
	NRF_LOG_INFO("ESPAR INIT DONE");
	espar_start();
	NRF_LOG_INFO("ESPAR START DONE");
#endif
}
