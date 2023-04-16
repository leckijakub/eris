#ifndef _ESPAR_DRIVER_INTERFACE_NRF52_
#define _ESPAR_DRIVER_INTERFACE_NRF52_
#include "sdk_common.h"
#include "nrf_gpio.h"
#include "nrf_log.h"

/* Printf configuration, if not available - comment out. */
#define PRINT_AVAILABLE
#define mcu_printf(...) printf(__VA_ARGS__)
#define mcu_gpio_pin_cfg(gpio) nrf_gpio_cfg_output(gpio)
#define mcu_gpio_pin_write(gpio, val) nrf_gpio_pin_write(gpio, val)

/* GPIOs for passive elements. */
#ifdef BOARD_PCA10056
#if ESPAR_STANDARD_V1
#define GPIO_NJG_CTRL_1                         NRF_GPIO_PIN_MAP(1, 1)  // P1.01
#define GPIO_NJG_CTRL_2                         NRF_GPIO_PIN_MAP(1, 2)  // P1.02
#define GPIO_NJG_CTRL_3                         NRF_GPIO_PIN_MAP(1, 3)  // P1.03
#define GPIO_NJG_CTRL_4                         NRF_GPIO_PIN_MAP(1, 4)  // P1.04
#define GPIO_NJG_CTRL_5                         NRF_GPIO_PIN_MAP(1, 5)  // P1.05
#define GPIO_NJG_CTRL_6                         NRF_GPIO_PIN_MAP(1, 6)  // P1.06
#define GPIO_NJG_CTRL_7                         NRF_GPIO_PIN_MAP(1, 7)  // P1.07
#define GPIO_NJG_CTRL_8                         NRF_GPIO_PIN_MAP(1, 8)  // P1.08
#define GPIO_NJG_CTRL_9                         NRF_GPIO_PIN_MAP(1, 10) // P1.10
#define GPIO_NJG_CTRL_10                        NRF_GPIO_PIN_MAP(1, 11) // P1.11
#define GPIO_NJG_CTRL_11                        NRF_GPIO_PIN_MAP(1, 12) // P1.12
#define GPIO_NJG_CTRL_12                        NRF_GPIO_PIN_MAP(1, 14) // P1.14
#define GPIO_LOGIC_SELECT                       NRF_GPIO_PIN_MAP(0, 10) // P0.10
#elif ESPAR_STANDARD_V2
#define GPIO_NJG_CTRL_1                         NRF_GPIO_PIN_MAP(1, 10) // P1.10
#define GPIO_NJG_CTRL_2                         NRF_GPIO_PIN_MAP(1, 11) // P1.11
#define GPIO_NJG_CTRL_3                         NRF_GPIO_PIN_MAP(1, 12) // P1.12
#define GPIO_NJG_CTRL_4                         NRF_GPIO_PIN_MAP(1, 13) // P1.13
#define GPIO_NJG_CTRL_5                         NRF_GPIO_PIN_MAP(1, 14) // P1.14
#define GPIO_NJG_CTRL_6                         NRF_GPIO_PIN_MAP(1, 15) // P1.15
#define GPIO_NJG_CTRL_7                         NRF_GPIO_PIN_MAP(1, 3)  // P1.03
#define GPIO_NJG_CTRL_8                         NRF_GPIO_PIN_MAP(1, 4)  // P1.04
#define GPIO_NJG_CTRL_9                         NRF_GPIO_PIN_MAP(1, 5)  // P1.05
#define GPIO_NJG_CTRL_10                        NRF_GPIO_PIN_MAP(1, 6)  // P1.06
#define GPIO_NJG_CTRL_11                        NRF_GPIO_PIN_MAP(1, 7)  // P1.07
#define GPIO_NJG_CTRL_12                        NRF_GPIO_PIN_MAP(1, 8)  // P1.08
#define GPIO_LOGIC_SELECT                       NRF_GPIO_PIN_MAP(0, 10) // P0.10
#elif ESPAR_DUAL_PASSIVE
#define GPIO_NJG_CTRL_1                         NRF_GPIO_PIN_MAP(1, 7)  // P1.07
#define GPIO_NJG_CTRL_2                         NRF_GPIO_PIN_MAP(1, 3)  // P1.03
#define GPIO_NJG_CTRL_3                         NRF_GPIO_PIN_MAP(1, 2)  // P1.02
#define GPIO_NJG_CTRL_4                         NRF_GPIO_PIN_MAP(1, 6)  // P1.06
#define GPIO_NJG_CTRL_5                         NRF_GPIO_PIN_MAP(1, 11) // P1.11
#define GPIO_NJG_CTRL_6                         NRF_GPIO_PIN_MAP(1, 12) // P1.12
#define GPIO_NJG_CTRL_7                         NRF_GPIO_PIN_MAP(1, 5)  // P1.05
#define GPIO_NJG_CTRL_8                         NRF_GPIO_PIN_MAP(1, 1)  // P1.01
#define GPIO_NJG_CTRL_9                         NRF_GPIO_PIN_MAP(1, 4)  // P1.04
#define GPIO_NJG_CTRL_10                        NRF_GPIO_PIN_MAP(1, 8)  // P1.08
#define GPIO_NJG_CTRL_11                        NRF_GPIO_PIN_MAP(1, 14) // P1.14
#define GPIO_NJG_CTRL_12                        NRF_GPIO_PIN_MAP(1, 10) // P1.10
#define GPIO_LOGIC_SELECT                       NRF_GPIO_PIN_MAP(0, 10) // P0.10
#endif

#elif defined(BOARD_PCA10059)
#define GPIO_NJG_CTRL_1                         NRF_GPIO_PIN_MAP(0, 31) // P0.31
#define GPIO_NJG_CTRL_2                         NRF_GPIO_PIN_MAP(0, 17) // P0.17
#define GPIO_NJG_CTRL_3                         NRF_GPIO_PIN_MAP(0, 20) // P0.20
#define GPIO_NJG_CTRL_4                         NRF_GPIO_PIN_MAP(0, 22) // P0.22
#define GPIO_NJG_CTRL_5                         NRF_GPIO_PIN_MAP(0, 24) // P0.24
#define GPIO_NJG_CTRL_6                         NRF_GPIO_PIN_MAP(1, 0)  // P1.00
#define GPIO_NJG_CTRL_7                         NRF_GPIO_PIN_MAP(0, 9)  // P0.09
#define GPIO_NJG_CTRL_8                         NRF_GPIO_PIN_MAP(1, 10) // P1.10
#define GPIO_NJG_CTRL_9                         NRF_GPIO_PIN_MAP(1, 13) // P1.13
#define GPIO_NJG_CTRL_10                        NRF_GPIO_PIN_MAP(1, 15) // P1.15
#define GPIO_NJG_CTRL_11                        NRF_GPIO_PIN_MAP(0, 2)  // P0.02
#define GPIO_NJG_CTRL_12                        NRF_GPIO_PIN_MAP(0, 29) // P0.29
#define GPIO_LOGIC_SELECT                       NRF_GPIO_PIN_MAP(0, 10) // P0.10

#elif defined(BOARD_DD)
#if ESPAR_STANDARD_V1
#define GPIO_NJG_CTRL_1                         NRF_GPIO_PIN_MAP(1, 15)  // P1.15
#define GPIO_NJG_CTRL_2                         NRF_GPIO_PIN_MAP(1, 13)  // P1.13
#define GPIO_NJG_CTRL_3                         NRF_GPIO_PIN_MAP(1, 10)  // P1.10
#define GPIO_NJG_CTRL_4                         NRF_GPIO_PIN_MAP(1, 12)  // P1.12
#define GPIO_NJG_CTRL_5                         NRF_GPIO_PIN_MAP(1, 9)   // P1.09
#define GPIO_NJG_CTRL_6                         NRF_GPIO_PIN_MAP(0, 12)  // P0.12
#define GPIO_NJG_CTRL_7                         NRF_GPIO_PIN_MAP(0, 11)  // P0.11
#define GPIO_NJG_CTRL_8                         NRF_GPIO_PIN_MAP(0, 14)  // P0.14
#define GPIO_NJG_CTRL_9                         NRF_GPIO_PIN_MAP(0, 15)  // P0.15
#define GPIO_NJG_CTRL_10                        NRF_GPIO_PIN_MAP(0, 13)  // P0.13
#define GPIO_NJG_CTRL_11                        NRF_GPIO_PIN_MAP(0, 17)  // P0.17
#define GPIO_NJG_CTRL_12                        NRF_GPIO_PIN_MAP(0, 21)  // P0.21
#define GPIO_LOGIC_SELECT                       NRF_GPIO_PIN_MAP(0, 10)  // P0.10
#elif ESPAR_STANDARD_V2
#define GPIO_NJG_CTRL_1                         NRF_GPIO_PIN_MAP(0, 15)  // P0.15
#define GPIO_NJG_CTRL_2                         NRF_GPIO_PIN_MAP(0, 13)  // P0.13
#define GPIO_NJG_CTRL_3                         NRF_GPIO_PIN_MAP(0, 17)  // P0.17
#define GPIO_NJG_CTRL_4                         NRF_GPIO_PIN_MAP(0, 22)  // P0.22
#define GPIO_NJG_CTRL_5                         NRF_GPIO_PIN_MAP(0, 21)  // P0.21
#define GPIO_NJG_CTRL_6                         NRF_GPIO_PIN_MAP(0, 20)  // P0.20
#define GPIO_NJG_CTRL_7                         NRF_GPIO_PIN_MAP(1, 10)  // P1.10
#define GPIO_NJG_CTRL_8                         NRF_GPIO_PIN_MAP(1, 12)  // P1.12
#define GPIO_NJG_CTRL_9                         NRF_GPIO_PIN_MAP(1, 9)   // P1.09
#define GPIO_NJG_CTRL_10                        NRF_GPIO_PIN_MAP(0, 12)  // P0.12
#define GPIO_NJG_CTRL_11                        NRF_GPIO_PIN_MAP(0, 11)  // P0.11
#define GPIO_NJG_CTRL_12                        NRF_GPIO_PIN_MAP(0, 14)  // P0.14
#define GPIO_LOGIC_SELECT                       NRF_GPIO_PIN_MAP(0, 10)  // P0.10
#endif
#define ANTENNA_ENABLE                          1
#define ANTENNA_ENABLE_PIN                      NRF_GPIO_PIN_MAP(1, 6)
#endif

#endif /* _ESPAR_DRIVER_INTERFACE_NRF52_ */
