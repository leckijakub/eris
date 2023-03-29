
#include "app_error.h"
#include "ble.h"
#include "ble_advertising.h"
#include "nrf_ble_scan.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "usb_serial.h"

#define SCAN_INTERVAL                                                          \
	0x00A0 /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                                                            \
	0x0050 /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_DURATION                                                          \
	0x0000 /**< Timout when scanning. 0x0000 disables timeout. */

#define APP_BLE_CONN_CFG_TAG                                                   \
	1 /**< A tag identifying the SoftDevice BLE configuration. */

NRF_BLE_SCAN_DEF(m_scan); /**< Scanning module instance. */

static char const m_target_periph_name[] =
    "ESPAR_CLIENT"; /**< Name of the device we try to connect to. This name is
		       searched in the scan report data*/

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
	uint8_t const *p_parsed_name;
	uint16_t data_offset = 0;
	// uint16_t target_name_len = strlen(m_target_periph_name);
	uint16_t target_name_len = 5;
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
		USB_SER_PRINT("%s", log_buffer);
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
		p_parsed_name = &adv_report->data.p_data[data_offset];
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
		USB_SER_PRINT("%s", log_buffer);

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

void master_init() { scan_init(); }
