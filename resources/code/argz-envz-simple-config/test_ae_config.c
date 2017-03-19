#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "ae_config.h"

static const char string[] = "foo=yaaa\n\
bar=woops\n\
baz=hop hop hop\n\
key with spaces = value with spaces \n\
# a comment doesn't necessarily start with an #, but it's clearer this way\n\
";
static const size_t size = sizeof(string) / sizeof(*string);

#define assert_str_equal(str, cmp) assert(strcmp((str), (cmp)) == 0)

static void test_conf(const struct ae_config *conf)
{
	puts(__func__);

	assert_str_equal(ae_config_get(conf, "foo"), "yaaa");
	assert_str_equal(ae_config_get(conf, "bar"), "woops");
	assert_str_equal(ae_config_get(conf, "baz"), "hop hop hop");
	assert_str_equal(ae_config_get(conf, "key with spaces "),
			" value with spaces ");
	assert(ae_config_get(conf, "non-existent key") == NULL);
}

static void test_string(void)
{
	struct ae_config conf;

	puts(__func__);

	assert(ae_config_read_from_string(&conf, string) == 0);
	test_conf(&conf);
	ae_config_cleanup(&conf);
}

static void file_cleanup(FILE **f)
{
	if (f == NULL || *f == NULL)
		return;

	fclose(*f);
	*f = NULL;
}

static void test_file(void)
{
	const char *path = "/tmp/" __FILE__ ".test";
	struct ae_config conf;
	FILE __attribute__((cleanup(file_cleanup)))*f = NULL;

	puts(__func__);

	assert((f = fopen(path, "wbe")) != NULL);
	assert(fwrite(string, sizeof(char), size, f) == size);
	assert(fflush(f) == 0);
	assert(ae_config_read(&conf, path) == 0);
	test_conf(&conf);
	ae_config_cleanup(&conf);
}

int main(void)
{
	puts(__FILE__);

	test_string();
	test_file();

	return EXIT_SUCCESS;
}
