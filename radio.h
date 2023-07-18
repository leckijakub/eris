#include <stdint.h>
#include <stdbool.h>

struct radio_packet_t {
	uint32_t data;
	uint32_t rssi;
};

void radio_init();
struct radio_packet_t read_packet();
void send_packet(uint32_t packet_to_send);
bool radio_power_level_valid(uint8_t power);
void radio_unmodulated_tx_carrier(uint8_t txpower, uint8_t channel);
void radio_disable();
void radio_rx(void (*packet_handler)(struct radio_packet_t));
void radio_tx(void (*packet_handler)(struct radio_packet_t*));
