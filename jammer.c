#include "nrf.h"
#include "radio.h"
#include <stdbool.h>
#include <stdint.h>
#include "nrf_log.h"
// #include "usb_serial.h"

static bool jammer_enabled = false;

void jammer_init(void)
{
	radio_init();
	//
}

int jammer_start(uint8_t power_level)
{
	if (!radio_power_level_valid(power_level)) {
		// USB_SER_PRINT("Invalid power level\r\n");
		NRF_LOG_INFO("Invalid power level");
		return 0;
	}
	radio_unmodulated_tx_carrier(power_level, 80UL);
	jammer_enabled = true;
	return 1;
}
void jammer_stop(void)
{
	jammer_enabled = false;
	radio_disable();
}

void jammer_handler()
{
	if (!jammer_enabled)
		return;
}
