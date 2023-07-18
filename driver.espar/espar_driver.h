#ifndef _ESPAR_DRIVER_H__
#define _ESPAR_DRIVER_H__
#include <stdbool.h>
#include <stdint.h>
#include "espar_driver_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPIOs for passive elements. */
#define NUMBER_OF_PASSIVE                       12

#if USE_3DIR_CHARACTERISTICS
#define NUMBER_OF_3DIR_CHAR                     12
#else
#define NUMBER_OF_3DIR_CHAR                     0
#endif

#if USE_5DIR_CHARACTERISTICS
#define NUMBER_OF_5DIR_CHAR                     12
#else
#define NUMBER_OF_5DIR_CHAR                     0
#endif

#if USE_8DIR_CHARACTERISTICS
#define NUMBER_OF_8DIR_CHAR                     12
#else
#define NUMBER_OF_8DIR_CHAR                     0
#endif

#if !(USE_3DIR_CHARACTERISTICS) && !(USE_5DIR_CHARACTERISTICS) && !(USE_8DIR_CHARACTERISTICS)
#error No characteristics defined!
#endif



/* Characteristics and key states. */
#define NUMBER_OF_CHARACTERISTICS               (NUMBER_OF_3DIR_CHAR + NUMBER_OF_5DIR_CHAR + NUMBER_OF_8DIR_CHAR)
#define R                                       0 // Reflector
#define D                                       1 // Director

extern uint8_t current_char_num;

/**
 * typedef characteristic - Describes a single characteristic.
 * @param passive Setting for passive elements, (e.g. {R, D, D, ..., R}).
 *
 * This type describes a single characteristic. It has an array of passive 
 * elements. Passive elements can act as reflectors (R) or directors (D).
 *
 * For example:
 * {D, D, D, D, D, R, R, R, R, R, R, R}
 * would mean:
 *
 *                 D2
 *             D1     D3
 *          R12         D4
 *         R11     o     D5
 *          R10         R6
 *            R9      R7
 *                R8
 *
 */
typedef struct characteristic
{
    uint8_t passive[NUMBER_OF_PASSIVE];
} characteristic;

extern uint8_t passive_gpios[NUMBER_OF_PASSIVE];

/* Declaration of functions. */
/**
 * espar_init() - Initiates ESPAR antenna.
 *
 * This function configures all GPIOs and sets them to zero
 * by using espar_zero_all() function. It also initiates connection
 * to antenna if ANTENNA_ENABLE is defined as 1.
 */
void espar_init(void);

/**
 * espar_driver_info() - Prints (if possible) ESPAR driver info.
 *
 * This function prints status of all macros for the driver,
 * for example type of ESPAR defined and characteristics to be used.
 */
void espar_driver_info(void);

/**
 * espar_set_passive_element() - Sets passive element.
 * @param element Which passive element to set.
 * @param value Which value to set (R or D).
 */
void espar_set_passive_element(uint8_t element, uint8_t value);

/**
 * espar_zero_all() - Resets characteristic to empty.
 *
 * This function resets all pins by setting them to 0
 * except ABC which is set to 111.
 */
void espar_zero_all(void);

/**
 * espar_set_characteristic() - Sets whole saved characteristic.
 * @param char_num Which characteristic to set, must be
 *            between 0 and NUMBER_OF_CHARACTERISTICS.
 *
 * @return Status
 * @retval 0: Success
 * @retval 1: Error
 *
 * This function resets all elements and sets a saved characteristic.
 */
bool espar_set_characteristic(uint8_t char_num);

    /**
     * espar_characteristic() - Sets next characteristic from list.
     */
void espar_next_characteristic();

/**
 * espar_previous_characteristic() - Sets next previous from list.
 */
void espar_previous_characteristic();

/**
 * espar_negate_characteristic() - Negates current characteristic.
 *
 * This function negates all passive elements of characteristic.
 * The ones that were reflectors will become directors, and the
 * other way around.
 */
void espar_negate_characteristic(void);

/**
 * espar_print_characteristic() - Prints current characteristic.
 */
void espar_print_characteristic(void);

/**
 * espar_all_reflectors() - Sets all elements as reflectors.
 */
void espar_all_reflectors(void);

/**
 * espar_all_directors() - Sets all elements as directors.
 */
void espar_all_directors(void);

/**
 * espar_set_logic() - Sets logic for antenna RF keys.
 * @param logic Logic 0 - negative, 1 - positive.
 *
 * @return Status
 * @retval 0: Success
 * @retval 1: Error
 *
 */
bool espar_set_logic(uint8_t logic);

/**
 * espar_get_current_characteristic() - Get current characteristic.
 *
 * @return Current characteristic.
 *
 */
characteristic espar_get_current_characteristic(void);

/**
 * espar_get_current_characteristic_num() - Get current characteristic number.
 *
 * @return Current characteristic number.
 *
 */
uint8_t espar_get_current_characteristic_num(void);

void espar_set_custom_characteristic(characteristic espar_char);

#ifdef __cplusplus
}
#endif

#endif // _ESPAR_DRIVER_H__
