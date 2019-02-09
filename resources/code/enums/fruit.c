#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <printf.h>

#include "fruit.h"

static int fruit_modifier;

static int fruit_printf_function(FILE *stream, const struct printf_info *info,
		const void *const *args)
{
	enum fruit fruit;

	if (!(info->user & fruit_modifier))
		return -2;

	fruit = **((int **)args);

	return fprintf(stream, "%s", fruit_to_string(fruit));
}

static int fruit_printf_arginfo_size_function(
		__attribute__((unused)) const struct printf_info *info,
		size_t n, int *argtypes, int *size)
{
	if (n > 0)
		*argtypes = PA_INT;
	*size = 1;

	return 1;
}

bool fruit_is_valid(enum fruit fruit)
{
	return fruit >= FRUIT_FIRST && fruit < FRUIT_COUNT;
}

const char *fruit_to_string(enum fruit fruit)
{
	switch (fruit) {
	case FRUIT_APPLE:
		return "apple";

	case FRUIT_ORANGE:
		return "orange";

	case FRUIT_CRANBERRY:
		return "cranberry";

	case FRUIT_INVALID:
		break;
	}

	return "(invalid)";
}

enum fruit fruit_from_string(const char *fruit_name)
{
	enum fruit fruit;

	for (fruit = FRUIT_FIRST; fruit < FRUIT_COUNT; fruit++)
		if (strcmp(fruit_name, fruit_to_string(fruit)) == 0)
			return fruit;

	return FRUIT_INVALID;
}

static __attribute__((constructor)) void fruit_init(void)
{
	register_printf_specifier('d', fruit_printf_function,
			fruit_printf_arginfo_size_function);
	fruit_modifier = register_printf_modifier(L"dfruit");
}
