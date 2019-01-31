#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <printf.h>

#include "fruit.h"

static const char *valid_to_str(bool b)
{
	return b ? "valid" : "invalid";
}

int main(void) {
	enum fruit fruit1 = 12;
	enum fruit fruit2 = FRUIT_CRANBERRY;
	const char *fruit_name;

	printf("fruit1: %s, fruit2: %s\n", valid_to_str(fruit_is_valid(fruit1)),
			valid_to_str(fruit_is_valid(fruit2)));

	printf("fruit2 is a %s\n", fruit_to_string(fruit2));
	fruit_name = "cranberry";

	printf("Value for %s is %d\n", fruit_name,
			fruit_from_string(fruit_name));

	printf("fruit2 really is a %"FRUIT_PRINT"\n", fruit2);

	/*
	 * our modifications on the `d` printf modifier aren't used when the
	 * "dfruit" modifier isn't specified:
	 */
	printf("%d\n", 42);

	return EXIT_SUCCESS;
}
