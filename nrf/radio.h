#include <stdint.h>

void radio_init();
uint32_t read_packet();
void send_packet(uint32_t packet_to_send);
void radio_unmodulated_tx_carrier(uint8_t txpower, uint8_t channel);
void radio_disable();
