#include "usb_serial.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_core.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_string_desc.h"

#include "boards.h"

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const *p_inst,
				    app_usbd_cdc_acm_user_event_t event);

/**
 * @brief CDC_ACM class instance
 * */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm, cdc_acm_user_ev_handler,
			    CDC_ACM_COMM_INTERFACE, CDC_ACM_DATA_INTERFACE,
			    CDC_ACM_COMM_EPIN, CDC_ACM_DATA_EPIN,
			    CDC_ACM_DATA_EPOUT,
			    APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

static char m_rx_buffer[READ_SIZE]; /** @brief Used to read through CDC */
static char m_tx_buffer[WRITE_SIZE]; /** @brief Used to send through CDC */
static char
	usb_ser_rx_buffer[512]; /** @brief Used to buffer received data from CDC */
static char usb_ser_tx_buffer[512]; /** @brief Used to buffer data to send if
					write function didn' finished */
static char usb_ser_log_buffer[512]; /** @brief Used to place formatted string
					using @ref USB_SER_PRINT macro */
volatile static size_t usb_ser_rx_size =
	0; /** @brief Stores current size of @ref usb_ser_rx_buffer buffer */
volatile static size_t usb_ser_tx_size =
	0; /** @brief Stores current size of @ref usb_ser_tx_buffer buffer */
volatile static bool tx_in_progres =
	false; /** @brief Indicates if CDC is occupied by write operation */
volatile static bool usb_ser_opened =
	false; /** @brief Indicates if CDC device is ready to be used */

static void usb_ser_write_directly(char *buf, size_t size)
{
	if (size > WRITE_SIZE) {
		return;
	}
	memcpy(m_tx_buffer, buf, size);
	app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, size);
	bsp_board_led_on(BSP_BOARD_LED_1);
	tx_in_progres = true;
}

static size_t usb_ser_flush()
{
	size_t write_bytes;

	if (usb_ser_tx_size <= 0 || tx_in_progres)
		return 0;
	usb_ser_write_directly(usb_ser_tx_buffer, usb_ser_tx_size);
	write_bytes = usb_ser_tx_size;
	usb_ser_tx_size = 0;
	return write_bytes;
}

void usb_ser_enable()
{
	if (usb_ser_opened) {
		return;
	}
	usb_ser_opened = true;
	usb_ser_flush();
}

void usb_ser_disable()
{
	usb_ser_opened = false;
}

void usb_ser_write(char *buf, size_t size)
{
	if (tx_in_progres || !usb_ser_opened) {
		memcpy(usb_ser_tx_buffer + usb_ser_tx_size, buf, size);
		usb_ser_tx_size += size;
	} else {
		usb_ser_write_directly(buf, size);
	}
}

static void (*usb_ser_input_parser)(char *, size_t) = NULL;

/**
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const *p_inst,
				    app_usbd_cdc_acm_user_event_t event)
{
	// app_usbd_cdc_acm_t const * p_cdc_acm =
	// app_usbd_cdc_acm_class_get(p_inst);

	switch (event) {
	case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN: {
		/*Setup first transfer*/
		ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
						       m_rx_buffer, READ_SIZE);
		UNUSED_VARIABLE(ret);
		NRF_LOG_INFO("ACM PORT OPENED\r\n");
		break;
	}
	case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
		usb_ser_disable();
		break;
	case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
		bsp_board_led_off(BSP_BOARD_LED_1);
		tx_in_progres = false;
		usb_ser_flush();
		break;
	case APP_USBD_CDC_ACM_USER_EVT_RX_DONE: {
		ret_code_t ret;
		size_t read_bytes;
		usb_ser_rx_size = 0;

		do {
			read_bytes = app_usbd_cdc_acm_rx_size(&m_app_cdc_acm);
			memcpy(&usb_ser_rx_buffer[usb_ser_rx_size], m_rx_buffer,
			       read_bytes);
			usb_ser_rx_size += read_bytes;

			/* Fetch data until internal buffer is empty */
			ret = app_usbd_cdc_acm_read(&m_app_cdc_acm, m_rx_buffer,
						    READ_SIZE);
		} while (ret == NRF_SUCCESS);
		usb_ser_input_parser(usb_ser_rx_buffer, usb_ser_rx_size);

		usb_ser_rx_size = 0;

		break;
	}
	default:
		break;
	}
}
static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
	switch (event) {
	case APP_USBD_EVT_DRV_SUSPEND:
		break;
	case APP_USBD_EVT_DRV_RESUME:
		break;
	case APP_USBD_EVT_STARTED:
		NRF_LOG_INFO("USBD EVT STARTED\r\n");
		break;
	case APP_USBD_EVT_STOPPED:
		app_usbd_disable();
		break;
	case APP_USBD_EVT_POWER_DETECTED:
		if (!nrf_drv_usbd_is_enabled()) {
			NRF_LOG_INFO("USB POWER DETECTED\r\n");
			app_usbd_enable();
		}
		break;
	case APP_USBD_EVT_POWER_REMOVED:
		NRF_LOG_INFO("USB POWER REMOVED\r\n");
		app_usbd_stop();
		break;
	case APP_USBD_EVT_POWER_READY:
		NRF_LOG_INFO("USBD EVT POWER READY\r\n");
		app_usbd_start();
		break;
	default:
		break;
	}
}

void usb_ser_init(void(*input_handler))
{
	ret_code_t ret;
	static const app_usbd_config_t usbd_config = {
		.ev_state_proc = usbd_user_ev_handler
	};
	app_usbd_serial_num_generate();

	ret = app_usbd_init(&usbd_config);
	APP_ERROR_CHECK(ret);
	NRF_LOG_INFO("USBD init done");

	app_usbd_class_inst_t const *class_cdc_acm =
		app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
	ret = app_usbd_class_append(class_cdc_acm);
	APP_ERROR_CHECK(ret);
	NRF_LOG_INFO("USBD ACM init done\r\n");

	usb_ser_input_parser = input_handler;
	app_usbd_enable();
	NRF_LOG_INFO("USBD Enabled\r\n");
	app_usbd_start();
	NRF_LOG_INFO("USBD Started\r\n");
}

char *usb_ser_log_get_txbuf()
{
	return usb_ser_log_buffer;
}

bool usb_ser_events_process()
{
	return app_usbd_event_queue_process();
}
