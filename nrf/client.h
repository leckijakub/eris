#include <stdint.h>

void client_init(void);
int client_start(uint8_t power_level);
void client_stop(void);
void client_handler();
