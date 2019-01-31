#ifndef FRUITS_H
#define FRUITS_H
#include <stdbool.h>

/**
 * @def FRUIT_PRINT
 * @brief Printf format specifier for printing fruits.
 */
#define FRUIT_PRINT "dfruitd"

/**
 * @enum fruit
 * @brief Eat 5 fruits or vegetables a day.
 * Pizzas don't count.
 */
enum fruit {
	FRUIT_FIRST,			/**< Lower bound value for fruits */

	FRUIT_APPLE = FRUIT_FIRST,	/**< Unaffordable brand */
	FRUIT_ORANGE,			/**< Telecommunication brand */
	FRUIT_CRANBERRY,		/**< Food for rock bands */

	FRUIT_COUNT,			/**< Upper bound value for fruits */
	FRUIT_INVALID = FRUIT_COUNT	/**< Invalid value for errors */
};

/**
 * @brief Tests wether a value corresponds to a valid fruit value.
 * @param fruit Fruit value to test.
 * @return true if the value is valid, false if not.
 */
bool fruit_is_valid(enum fruit fruit);

/**
 * @brief Converts a fruit enum value into a human readable string.
 * @param fruit Enum value.
 * @return String representation for the fruit.
 */
const char *fruit_to_string(enum fruit fruit);

/**
 * @brief Retrieves the enum value of a fruit knowing its name.
 * @param fruit_name Name of the fruit to convert to its enum value.
 * @return Enum value for the fruit.
 */
enum fruit fruit_from_string(const char *fruit_name);

#endif
