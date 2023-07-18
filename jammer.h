#include <stdint.h>
void jammer_init(void);
int jammer_start(uint8_t power_level);
void jammer_stop(void);
void jammer_handler();
