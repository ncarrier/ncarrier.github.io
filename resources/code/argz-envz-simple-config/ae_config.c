#include <stdlib.h>
#include <stdio.h>
#include <argz.h>
#include <envz.h>

#include "ae_config.h"

static void file_cleanup(FILE **f)
{
	if (f == NULL || *f == NULL)
		return;

	fclose(*f);
	*f = NULL;
}

static void string_cleanup(char **s)
{
	if (s == NULL || *s == NULL)
		return;

	free(*s);
	*s = NULL;
}

int ae_config_read(struct ae_config *conf, const char *path)
{
	int ret;
	char __attribute__((cleanup(string_cleanup)))*string = NULL;
	FILE __attribute__((cleanup(file_cleanup)))*f = NULL;
	long size;
	size_t sret;

	f = fopen(path, "rbe");
	if (f == NULL)
		return -errno;

	/* compute the size of the file */
	ret = fseek(f, 0, SEEK_END);
	if (ret == -1)
		return -errno;
	size = ftell(f);
	if (ret == -1)
		return -errno;
	ret = fseek(f, 0, SEEK_SET);
	if (ret == -1)
		return -errno;

	/* read all */
	string = calloc(size, 1);
	if (string == NULL)
		return -errno;

	sret = fread(string, 1, size, f);
	if (sret < (size_t)size)
		return feof(f) ? -EIO : ret;

	return ae_config_read_from_string(conf, string);
}

int ae_config_read_from_string(struct ae_config *conf, const char *string)
{
	return -argz_create_sep(string, '\n', &conf->argz, &conf->len);
}

const char *ae_config_get(const struct ae_config *conf, const char *key)
{
	return envz_get(conf->argz, conf->len, key);
}

void ae_config_cleanup(struct ae_config *conf)
{
	free(conf->argz);
	memset(conf, 0, sizeof(*conf));
}
