#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

static void cleanup_string(char **string)
{
	if (string == NULL)
		return;

	printf("%s\n", __func__);

	free(*string);
	*string = NULL;
}

/*
 * takes a frobnicated buffer containing a c-string, un-frobnicates it and
 * converts the result to an integer
 */
static int parse_frobnicated_int(const char *src, size_t size, int *retval)
{
	char __attribute__((cleanup(cleanup_string)))*string = NULL;
	long value;
	char *endptr;

	string = calloc(size, sizeof(*string));
	if (string == NULL)
		return -errno;
	memcpy(string, src, size);

	/* check src is a string, to avoid segfault in strtol */
	if ((string[size - 1] ^ 42) != '\0')
		return -EINVAL;

	/* restore and parse the original string */
	memfrob(string, size);
	value = strtol(string, &endptr, 0);
	/* the entire string is _not_ valid */
	if (*endptr != '\0')
		return -EINVAL;
	*retval = value;

	return 0;
}

int main(void)
{
	unsigned i;
	const char string_int[] = "125445";
	size_t size = sizeof(string_int) / sizeof(*string_int);
	char frobnicated_string_int[size];
	int value;

	memcpy(frobnicated_string_int, string_int, size);
	memfrob(frobnicated_string_int, size);
	printf("string int after frobnication:\n\t");
	for (i = 0; i < size; i++)
		printf("%#x ", frobnicated_string_int[i]);
	puts("");

	parse_frobnicated_int(frobnicated_string_int, size, &value);
	printf("parsed int: %d\n", value);

	return EXIT_SUCCESS;
}
