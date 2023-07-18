#include "espar_driver.h"
#include "espar_driver_interface.h"
#include <stdbool.h>
#include <stdint.h>

/* Array of all characteristics. */
#if (ESPAR_STANDARD_V1) | (ESPAR_STANDARD_V2)
static characteristic characteristics[NUMBER_OF_CHARACTERISTICS] = {

#if USE_3DIR_CHARACTERISTICS
    {{D, D, D, R, R, R, R, R, R, R, R, R}}, // Characteristic  1
    {{R, D, D, D, R, R, R, R, R, R, R, R}}, // Characteristic  2
    {{R, R, D, D, D, R, R, R, R, R, R, R}}, // Characteristic  3
    {{R, R, R, D, D, D, R, R, R, R, R, R}}, // Characteristic  4
    {{R, R, R, R, D, D, D, R, R, R, R, R}}, // Characteristic  5
    {{R, R, R, R, R, D, D, D, R, R, R, R}}, // Characteristic  6
    {{R, R, R, R, R, R, D, D, D, R, R, R}}, // Characteristic  7
    {{R, R, R, R, R, R, R, D, D, D, R, R}}, // Characteristic  8
    {{R, R, R, R, R, R, R, R, D, D, D, R}}, // Characteristic  9
    {{R, R, R, R, R, R, R, R, R, D, D, D}}, // Characteristic 10
    {{D, R, R, R, R, R, R, R, R, R, D, D}}, // Characteristic 11
    {{D, D, R, R, R, R, R, R, R, R, R, D}}, // Characteristic 12
#endif					  /* USE_3DIR_CHARACTERISTICS */
#if USE_5DIR_CHARACTERISTICS
    {{D, D, D, D, D, R, R, R, R, R, R, R}}, // Characteristic  1
    {{R, D, D, D, D, D, R, R, R, R, R, R}}, // Characteristic  2
    {{R, R, D, D, D, D, D, R, R, R, R, R}}, // Characteristic  3
    {{R, R, R, D, D, D, D, D, R, R, R, R}}, // Characteristic  4
    {{R, R, R, R, D, D, D, D, D, R, R, R}}, // Characteristic  5
    {{R, R, R, R, R, D, D, D, D, D, R, R}}, // Characteristic  6
    {{R, R, R, R, R, R, D, D, D, D, D, R}}, // Characteristic  7
    {{R, R, R, R, R, R, R, D, D, D, D, D}}, // Characteristic  8
    {{D, R, R, R, R, R, R, R, D, D, D, D}}, // Characteristic  9
    {{D, D, R, R, R, R, R, R, R, D, D, D}}, // Characteristic 10
    {{D, D, D, R, R, R, R, R, R, R, D, D}}, // Characteristic 11
    {{D, D, D, D, R, R, R, R, R, R, R, D}}, // Characteristic 12
#endif					  /* USE_5DIR_CHARACTERISTICS */
#if USE_8DIR_CHARACTERISTICS
    {{D, D, D, D, D, D, D, D, R, R, R, R}}, // Characteristic  1
    {{R, D, D, D, D, D, D, D, D, R, R, R}}, // Characteristic  2
    {{R, R, D, D, D, D, D, D, D, D, R, R}}, // Characteristic  3
    {{R, R, R, D, D, D, D, D, D, D, D, R}}, // Characteristic  4
    {{R, R, R, R, D, D, D, D, D, D, D, D}}, // Characteristic  5
    {{D, R, R, R, R, D, D, D, D, D, D, D}}, // Characteristic  6
    {{D, D, R, R, R, R, D, D, D, D, D, D}}, // Characteristic  7
    {{D, D, D, R, R, R, R, D, D, D, D, D}}, // Characteristic  8
    {{D, D, D, D, R, R, R, R, D, D, D, D}}, // Characteristic  9
    {{D, D, D, D, D, R, R, R, R, D, D, D}}, // Characteristic 10
    {{D, D, D, D, D, D, R, R, R, R, D, D}}, // Characteristic 11
    {{D, D, D, D, D, D, D, R, R, R, R, D}}, // Characteristic 12
#endif					  /* USE_8DIR_CHARACTERISTICS */
};
#elif (ESPAR_DUAL_PASSIVE)
static characteristic characteristics[NUMBER_OF_CHARACTERISTICS] = {
    {{D, R, R, R, R, R, D, D, R, D, R, D}}, // Characteristic  1
    {{R, D, R, R, R, R, D, D, D, R, D, R}}, // Characteristic  2
    {{R, R, D, R, R, R, R, D, D, D, R, D}}, // Characteristic  3
    {{R, R, R, D, R, R, D, R, D, D, D, R}}, // Characteristic  4
    {{R, R, R, R, D, R, R, D, R, D, D, D}}, // Characteristic  5
    {{R, R, R, R, R, D, D, R, D, R, D, D}}, // Characteristic  6
    {{D, D, R, R, R, D, R, R, R, D, R, R}}, // Characteristic  7
    {{D, D, D, R, R, R, R, R, R, R, D, R}}, // Characteristic  8
    {{R, D, D, D, R, R, R, R, R, R, R, D}}, // Characteristic  9
    {{R, R, D, D, D, R, D, R, R, R, R, R}}, // Characteristic 10
    {{R, R, R, D, D, D, R, D, R, R, R, R}}, // Characteristic 11
    {{D, R, R, R, D, D, R, R, D, R, R, R}}, // Characteristic 12
};
#endif

characteristic current_characteristic;
uint8_t current_char_num = 1;

uint8_t passive_gpios[NUMBER_OF_PASSIVE] = {
    GPIO_NJG_CTRL_1, GPIO_NJG_CTRL_2,  GPIO_NJG_CTRL_3,	 GPIO_NJG_CTRL_4,
    GPIO_NJG_CTRL_5, GPIO_NJG_CTRL_6,  GPIO_NJG_CTRL_7,	 GPIO_NJG_CTRL_8,
    GPIO_NJG_CTRL_9, GPIO_NJG_CTRL_10, GPIO_NJG_CTRL_11, GPIO_NJG_CTRL_12};

static void gpio_pin_write(uint8_t gpio, uint8_t val)
{
	mcu_gpio_pin_write(gpio, val);
}
static void gpio_pin_cfg(uint8_t gpio) { mcu_gpio_pin_cfg(gpio); }

static void antenna_connection_init(void)
{
#if defined(ANTENNA_ENABLE)
	nrf_gpio_cfg_output(ANTENNA_ENABLE_PIN);
	nrf_gpio_pin_write(ANTENNA_ENABLE_PIN, 0);
#endif
}

void espar_init(void)
{
	uint8_t i;
	for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
		gpio_pin_cfg(passive_gpios[i]);
	}

	espar_zero_all();
	antenna_connection_init();
}

void espar_driver_info(void)
{
#ifdef PRINT_AVAILABLE
#if defined(ESPAR_STANDARD_V1)
	mcu_printf("ESPAR STANDARD = %d", ESPAR_STANDARD_V1);
#endif
#if defined(ESPAR_STANDARD_V2)
	mcu_printf("ESPAR STANDARD V2 = %d", ESPAR_STANDARD_V2);
#endif
#if defined(ESPAR_DUAL_PASSIVE)
	mcu_printf("ESPAR DUAL_PASSIVE = %d", ESPAR_DUAL_PASSIVE);
#endif
#if defined(USE_3DIR_CHARACTERISTICS)
	mcu_printf("USE_3DIR_CHARACTERISTICS = %d", USE_3DIR_CHARACTERISTICS);
#endif
#if defined(USE_5DIR_CHARACTERISTICS)
	mcu_printf("USE_5DIR_CHARACTERISTICS = %d", USE_5DIR_CHARACTERISTICS);
#endif
#if defined(USE_8DIR_CHARACTERISTICS)
	mcu_printf("USE_8DIR_CHARACTERISTICS = %d", USE_8DIR_CHARACTERISTICS);
#endif
#endif
}

void espar_set_passive_element(uint8_t element, uint8_t value)
{
	gpio_pin_write(passive_gpios[element], value);
	current_characteristic.passive[element] = value;
}

void espar_zero_all(void)
{
	uint8_t i;
	for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
		gpio_pin_write(passive_gpios[i], 0);
		current_characteristic.passive[i] = 0;
	}
}

void espar_set_custom_characteristic(characteristic espar_char)
{
	espar_zero_all();
	for (int i = 0; i < NUMBER_OF_PASSIVE; i++) {
		espar_set_passive_element(i, espar_char.passive[i]);
	}
}

bool espar_set_characteristic(uint8_t char_num)
{
	if (char_num > 0 && char_num <= NUMBER_OF_CHARACTERISTICS) {
		espar_zero_all();

		current_characteristic = characteristics[char_num - 1];
		current_char_num = char_num;

		/* Setting passive elements. */
		uint8_t i;
		for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
			espar_set_passive_element(
			    i, current_characteristic.passive[i]);
		}
		return false;
	} else {
		return true;
	}
}

void espar_next_characteristic()
{
	current_char_num++;
	if (current_char_num > NUMBER_OF_CHARACTERISTICS) {
		current_char_num = 1;
	}
	espar_set_characteristic(current_char_num);
}

void espar_previous_characteristic()
{
	current_char_num--;
	if (current_char_num < 1) {
		current_char_num = NUMBER_OF_CHARACTERISTICS;
	}
	espar_set_characteristic(current_char_num);
}

void espar_negate_characteristic(void)
{
	uint8_t i;
	/* Setting passive elements. */
	for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
		espar_set_passive_element(i,
					  !(current_characteristic.passive[i]));
	}
}

void espar_print_characteristic(void)
{
#ifdef PRINT_AVAILABLE
	uint8_t i;

	for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
		mcu_printf("P%d=%d ", i + 1, current_characteristic.passive[i]);
	}
	mcu_printf("\r\n");
#endif
}

void espar_all_reflectors(void)
{
	espar_zero_all();
	uint8_t i;
	for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
		espar_set_passive_element(i, R);
	}
}

void espar_all_directors(void)
{
	espar_zero_all();
	uint8_t i;
	for (i = 0; i < NUMBER_OF_PASSIVE; i++) {
		espar_set_passive_element(i, D);
	}
}

bool espar_set_logic(uint8_t logic)
{
	if (logic >= 0 && logic <= 1) {
		gpio_pin_write(GPIO_LOGIC_SELECT, logic);
		return false;
	}
	return true;
}

uint8_t espar_get_current_characteristic_num(void) { return current_char_num; }

characteristic espar_get_current_characteristic(void)
{
	return current_characteristic;
}
