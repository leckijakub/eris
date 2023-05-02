
#include "app_error.h"
#include "ble.h"
#include "ble_advertising.h"
#include "nrf_ble_scan.h"
// #include "boards.h"
// #include "nrf_log.h"
// #include "nrf_log_ctrl.h"
// #include "nrf_log_default_backends.h"
#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "espar_driver.h"
#include "radio.h"
#include "usb_serial.h"

#ifdef ESPAR_GENETIC
#include "espar_genetic.h"
#endif

static bool master_enabled = false;
bool espar_run = false;
#ifdef ESPAR_GENETIC
struct espar_gen_ctx eg_ctx;
#endif


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

void master_handler(void)
{
	uint32_t received = 0;
	if (!master_enabled){
		return;
	}
	received = read_packet();

	NRF_LOG_INFO("Packet was received");

	NRF_LOG_INFO("The contents of the package is %u",
		     (unsigned int)received);
	NRF_LOG_FLUSH();
	// set_char(0xffff);
}

void master_start(){
	master_enabled = true;
}

void master_stop(){
	master_enabled = false;
}

void master_init()
{
	radio_init();
#ifdef BOARD_DD
	espar_init();
	NRF_LOG_INFO("ESPAR INIT DONE");
	espar_start();
	set_char(0xffff);
	NRF_LOG_INFO("ESPAR START DONE");
#ifdef ESPAR_GENETIC
	espar_gen_init(&eg_ctx);
	NRF_LOG_INFO("ESPAR GENETIC INIT DONE");
#endif
#endif
}
