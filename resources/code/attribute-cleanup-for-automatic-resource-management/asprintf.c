#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

static void cleanup_string(char **s)
{
	if (s == NULL || *s == NULL)
		return;
	free(*s);
}

/* return NULL on error, with errno set */
static FILE *open_file(const char *dir, const char *name)
{
	int ret;
	char __attribute__((cleanup(cleanup_string))) *path = NULL;

	ret = asprintf(&path, "%s/%s", dir, name);
	if (ret == -1) {
		errno = ENOMEM;
		path = NULL;
		return NULL;
	}

	return fopen(path, "r");
}

int main(void)
{
	FILE *f;

	f = open_file(".", "test_file");
	if (f == NULL)
		error(EXIT_FAILURE, errno, "too bad, can't open file");
	printf("file opened successfully\n");
	fclose(f);

	return EXIT_SUCCESS;
}
