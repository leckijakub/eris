
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "espar_driver.h"
#include "usb_serial.h"

#define SCAN_INTERVAL                                                          \
	0x00A0 /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                                                            \
	0x0050 /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_DURATION                                                          \
	0x0000 /**< Timout when scanning. 0x0000 disables timeout. */

#define APP_BLE_CONN_CFG_TAG                                                   \
	1 /**< A tag identifying the SoftDevice BLE configuration. */

#define ESPAR_CHARACTERISTIC_TIME                                              \
	50 /* Time in ms between characteristic change. */

NRF_BLE_SCAN_DEF(m_scan); /**< Scanning module instance. */

static char const m_target_periph_name[] =
    "ESPAR_CLIENT"; /**< Name of the device we try to connect to. This name is
		       searched in the scan report data*/

bool espar_run = false;
bool scan_in_progress = false;
bool scan_done = false;
int8_t scan_rssi = 0;

/**@brief Function to start scanning.
 */
void scan_start(void)
{
	ret_code_t err_code;

	err_code = nrf_ble_scan_start(&m_scan);
	APP_ERROR_CHECK(err_code);
}
void scan_stop(void) { nrf_ble_scan_stop(); }

int sprint_address(char *log_buffer,
		   const ble_gap_evt_adv_report_t *p_adv_report)
{
	const uint8_t *mac = p_adv_report->peer_addr.addr;
	return sprintf(log_buffer, "MAC:%02x%02x%02x%02x%02x%02x\r\n", mac[0],
		       mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int sprint_name(char *log_buffer, const ble_gap_evt_adv_report_t *p_adv_report)
{
	uint16_t offset = 0;
	char name[64] = {0};

	uint16_t length = ble_advdata_search(
	    p_adv_report->data.p_data, p_adv_report->data.len, &offset,
	    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME);
	if (length == 0) {
		// Look for the short local name if it was not found as
		// complete.
		length = ble_advdata_search(p_adv_report->data.p_data,
					    p_adv_report->data.len, &offset,
					    BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME);
	}

	if (length != 0) {
		memcpy(name, &p_adv_report->data.p_data[offset], length);
		return sprintf(log_buffer, "name: %s\r\n", name);
	}
	return 0;
}

int sprint_manufacturer_data(char *log_buffer,
			     const ble_gap_evt_adv_report_t *p_adv_report)
{
	uint16_t offset = 0;
	uint16_t length = ble_advdata_search(
	    p_adv_report->data.p_data, p_adv_report->data.len, &offset,
	    BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA);
	uint16_t log_offset = 0;
	ble_advdata_manuf_data_t *rec_manuf_data =
	    (ble_advdata_manuf_data_t *)&p_adv_report->data.p_data[offset];
	if (length != 0) {
		log_offset += sprintf(log_buffer + log_offset,
				      "manufacturer ID: %04x\r\n",
				      rec_manuf_data->company_identifier);
		log_offset +=
		    sprintf(log_buffer + log_offset,
			    "manufacturer data size: %04x (Bytes)\r\n",
			    rec_manuf_data->data.size);
		log_offset += sprintf(
		    log_buffer + log_offset,
		    "manufacturer length reported: %04x (Bytes)\r\n", length);
		char data_string[1024] = {0};
		char *pos = data_string;
		for (int i = 0; i < length && i < 512; i++) {
			sprintf(pos, "%02x",
				p_adv_report->data.p_data[offset + i]);
			pos += 2;
		}

		return sprintf(log_buffer + log_offset,
			       "manufacturer data: %s\r\n", data_string) +
		       log_offset;
	}
	return 0;
}

/**@brief Function for handling Scaning events.
 *
 * @param[in]   p_scan_evt   Scanning event.
 */

static void scan_evt_handler(scan_evt_t const *p_scan_evt)
{
	ret_code_t err_code;
	char log_buffer[512] = {0};
	char *log_p = log_buffer;

	uint16_t parsed_name_len;
	// uint8_t const *p_parsed_name;
	uint16_t data_offset = 0;
	// uint16_t target_name_len = strlen(m_target_periph_name);
	// uint16_t target_name_len = 5;
	ble_gap_evt_adv_report_t const *adv_report;

	// USB_SER_PRINT("SCANNING HANDLER TRIGGERRED, ID: %d\r\n",
	// p_scan_evt->scan_evt_id);
	switch (p_scan_evt->scan_evt_id) {
	case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
		err_code = p_scan_evt->params.connecting_err.err_code;
		APP_ERROR_CHECK(err_code);
		break;

	case NRF_BLE_SCAN_EVT_FILTER_MATCH:
		log_p += sprintf(log_p, "\r\nFound BT device:\r\n");
		log_p += sprint_address(
		    log_p, p_scan_evt->params.filter_match.p_adv_report);
		log_p += sprint_name(
		    log_p, p_scan_evt->params.filter_match.p_adv_report);
		log_p +=
		    sprintf(log_p, "rssi: %d\r\n",
			    p_scan_evt->params.filter_match.p_adv_report->rssi);
		log_p += sprintf(
		    log_p, "channel: %d\r\n",
		    p_scan_evt->params.filter_match.p_adv_report->ch_index);
		log_p += sprint_manufacturer_data(
		    log_p, p_scan_evt->params.filter_match.p_adv_report);
		NRF_LOG_INFO("%s", log_buffer);

		scan_rssi = p_scan_evt->params.filter_match.p_adv_report->rssi;
		scan_done = true;
		break;
	case NRF_BLE_SCAN_EVT_NOT_FOUND:
		// USB_SER_PRINT("SCAN NOT FOUND\r\n");
		// uint16_t target_name_len = strlen(m_target_periph_name);

		adv_report = p_scan_evt->params.p_not_found;

		/* Scan encoded adv. payload for data of type
		 * BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME */
		parsed_name_len = ble_advdata_search(
		    adv_report->data.p_data, adv_report->data.len, &data_offset,
		    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME);

		/* Name found if parsed_name_len != 0 */
		if (!parsed_name_len) {
			break;
		}
		// p_parsed_name = &adv_report->data.p_data[data_offset];
		// compare only target_name_len chars of advertised name and
		// target name if(memcmp(m_target_periph_name, p_parsed_name,
		// target_name_len)== 0)
		// {
		log_p += sprintf(log_p, "\r\nFound Other BT device:\r\n");
		log_p += sprint_address(log_p, adv_report);
		log_p += sprint_name(log_p, adv_report);
		log_p += sprintf(log_p, "rssi: %d\r\n", adv_report->rssi);
		log_p +=
		    sprintf(log_p, "channel: %d\r\n", adv_report->ch_index);
		log_p += sprint_manufacturer_data(log_p, adv_report);
		// NRF_LOG_INFO("%s", log_buffer);

		// connect with peripheral
		// nrf_ble_scan_connect_with_target(&m_scan, adv_report);

		// }
		break;
	default:
		break;
	}
}

static void scan_init(void)
{
	ret_code_t err_code;
	nrf_ble_scan_init_t init_scan;

	memset(&init_scan, 0, sizeof(init_scan));
	// init_scan.connect_if_match = true;

	init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

	err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
	APP_ERROR_CHECK(err_code);

	// Setting filters for scanning.
	err_code = nrf_ble_scan_filters_enable(&m_scan,
					       NRF_BLE_SCAN_NAME_FILTER, false);
	APP_ERROR_CHECK(err_code);
	err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_NAME_FILTER,
					   m_target_periph_name);
	APP_ERROR_CHECK(err_code);
}

static void timer0_init(uint32_t delay_ms)
{
	NRF_LOG_INFO("TIMER 0\r\n");
	NRF_TIMER0->TASKS_STOP = 1;
	NRF_LOG_INFO("TIMER 1\r\n");
	// Create an Event-Task shortcut to clear Timer 1 on COMPARE[0] event.
	NRF_TIMER0->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled
			      << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
	NRF_LOG_INFO("TIMER 2\r\n");
	NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
	NRF_LOG_INFO("TIMER 3\r\n");
	NRF_TIMER0->BITMODE =
	    (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
	NRF_LOG_INFO("TIMER 4\r\n");
	NRF_TIMER0->PRESCALER = 4; // resolution of 1 us
	NRF_LOG_INFO("TIMER 5\r\n");
	NRF_TIMER0->INTENSET =
	    (TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos);
	NRF_LOG_INFO("TIMER 6\r\n");
	// Sample update must happen as soon as possible. The earliest possible
	// moment is MAX_SAMPLE_LEVELS ticks before changing the output duty
	// cycle.
	NRF_TIMER0->CC[0] = ((uint32_t)delay_ms * 1000) - 1;
	NRF_LOG_INFO("TIMER 7\r\n");
	NRF_TIMER0->CC[1] = 0;
	NRF_LOG_INFO("TIMER 8\r\n");
	NRF_TIMER0->TASKS_START = 1;
	NRF_LOG_INFO("TIMER 9\r\n");
}

void espar_start()
{
	espar_run = 1;
	// timer0_init(ESPAR_CHARACTERISTIC_TIME);
	// NRF_LOG_INFO("TIMER0 INIT DONE\r\n");
	espar_set_characteristic(0);
}

bool scan_result_available() { return scan_done; }

// int8_t get_scan_rssi()
// {
// 	int8_t rssi = 0;

// 	if (scan_in_progress) {
// 		return 0;
// 	}

// 	scan_start();
// 	NRF_LOG_INFO("SCAN STARTED");
// 	scan_in_progress = true;
// 	while (!scan_result_available()) {
// 		// wait for scan to finish
// 		// TODO: This should be more power-efficient (sleep)
// 	}
// 	scan_stop();
// 	scan_in_progress = false;
// 	scan_done = false;
// 	rssi = scan_rssi;
// 	scan_rssi = 0;
// 	return rssi;
// }

// characteristic espar_char = {.passive = { 0 }};
uint16_t jam_char = 0;
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
	for (z = 1>>11; z > 0; z >>= 1) {
		strcat(b, ((x & z) == z) ? "1" : "0");
	}

	return b;
}

// void TIMER0_IRQHandler(void)
void master_handler(void)
{
	int8_t rssi = 0;
	// if (NRF_TIMER0->EVENTS_COMPARE[0]) {
	// NRF_TIMER0->EVENTS_COMPARE[0] = 0;

	// NRF_LOG_INFO("Timer hit\r\n");
	if (espar_run) {
		if (jam_char < (1 << NUMBER_OF_PASSIVE) - 1) {
			// ####################################################
			int8_t rssi = 0;

			if (!scan_in_progress) {
				scan_start();
				scan_in_progress = true;
				NRF_LOG_INFO("SCAN STARTED");
				return;
			}
			if (!scan_result_available()) {
				return;
			}
			scan_stop();
			scan_in_progress = false;
			scan_done = false;
			rssi = scan_rssi;
			scan_rssi = 0;
			NRF_LOG_INFO("characteristic: %s, RSSI: %d\r\n",
				     espar_char_as_string(jam_char), rssi);
			set_char(++jam_char);
			// ####################################################
			// rssi = get_scan_rssi();
			// NRF_LOG_INFO("Hello");
		} else {
			jam_char = 0;
			set_char(jam_char);
		}
	} else {
		// Do nothing
	}
	// }

	// if (NRF_TIMER0->EVENTS_COMPARE[1]) {
	// 	NRF_TIMER0->EVENTS_COMPARE[1] = 0;
	// }
}

void master_init()
{
	scan_init();
	NRF_LOG_INFO("SCAN DONE\r\n");
	// NVIC_EnableIRQ(TIMER0_IRQn);
	// NRF_LOG_INFO("NVIC DONE\r\n");
	// __enable_irq();
	// NRF_LOG_INFO("IRQ ENABLE DONE\r\n");
	espar_init();
	NRF_LOG_INFO("ESPAR INIT DONE\r\n");
	espar_start();
	NRF_LOG_INFO("ESPAR START DONE\r\n");
}
