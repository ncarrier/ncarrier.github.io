---
layout: post
title:  "Argz / envz - simple configuration file format"
date:   2017-03-17 18:12:00
categories: C glibc
longtitle: "Lesser known glibc features"
author: Nicolas Carrier
description: A configuration file format with argz / envz.
---

Reading the [glibc manual][glibc] can reveal some hidden gems which can prove
useful from times to times.
One I recently stumbled across resides in the `argz.h` and `envz.h` headers.
In this article, I will present a simple, but quite common use case, which
is reading and writing a simple configuration file, consisting of key / value
pairs.

In this article, I'll start by proposing a basic API, in the form of a simple
string registry, loaded either from memory or from a file.
Then I'll implement a small test suite.
And I'll propose an implementation, based on the argz / envz APIs.

# Prerequisite

To build the code listed in this blog post, you'll only need a working version
of gcc, no special dependency is required.

# Overview

The project will consist in 3 files.
The [ae_config.h] header which is the "public" API, it's implementation,
[ae_config.c] and a test program [test_ae_config.c].

To compile it, I suggest doing:

        gcc *.c -o test_ae_config -Wall -Wextra -Werror -O0 -g3

`-Wall -Wextra -Werror` will enable a set of warnings which is a good starting
point for this kind of simple project and makes sure that they can't be missed.
`-O0 -g3` will make the debug with gdb less painful.
If you choose to build with another method, please be sure that the NDEBUG macro
isn't defined, otherwise, the `assert` macro will not be defined and
*test_ae_config.c* won't work.

# The API - [ae_config.h]

The argz / envz APIs store an array of nul-terminated strings into one char
array.
This is quite handy, because only one allocation is required and one level of
pointer indirection is saved.
But for this to work, an extra number is required, to store the overall storage
size.

Hence the configuration registry structure can be defined as:

        struct ae_config {
            char *argz;
            size_t len;
        };

We'll need some functions to populate the registry, for a start, we'll load the
config from either a file or from a string in memory.
We'll implement the former, using the latter:

        int ae_config_read(struct ae_config *conf, const char *path);
        int ae_config_read_from_string(struct ae_config *conf, const char *string);

These two functions will do some allocations, so we'll need a function to do
the cleanup:

        void ae_config_cleanup(struct ae_config *conf);

And of course, at least one query function is required to make this API useful:

        const char *ae_config_get(const struct ae_config *conf, const char *key);

This API is quite frugal, but can be quite easily extended.
For example, one may want to be able to create a config, item by item, or to
delete items, merge configs, iterate over the keys...
All of these are possible quite simply, as exploring the documentation of [argz]
and [envz] will show you.

# Automated tests suite - [test_ae_config.c]

The test program makes intensive use of the `assert` macro.
To ease it's usage in  this context, one may want to write some wrapper macros
for some test scenarios, like I did with the `assert_str_equal` macro, used to
check that two strings are equal as expected.

The center part is the `test_conf` function, testing that some expected keys are
present and with the expected value and that a non existent key is associated to
the NULL value:

        assert_str_equal(ae_config_get(conf, "foo"), "yaaa");
        assert_str_equal(ae_config_get(conf, "bar"), "woops");
        assert_str_equal(ae_config_get(conf, "baz"), "hop hop hop");
        assert_str_equal(ae_config_get(conf, "key with spaces "),
                " value with spaces ");
        assert(ae_config_get(conf, "non-existent key") == NULL);

To simplify the test, the configuration uses is a simple string, `string`.
This way we can test the config obtained from it with
`ae_config_read_from_string`, in `test_string`:

        assert(ae_config_read_from_string(&conf, string) == 0);
        test_conf(&conf);
        ae_config_cleanup(&conf);

and, in `test_file`, we can also test `ae_config_read` with the same data,
provided we write it to a file beforehand:

        assert((f = fopen(path, "wbe")) != NULL);
        assert(fwrite(string, sizeof(char), size, f) == size);
        assert(fflush(f) == 0);
        assert(ae_config_read(&conf, path) == 0);
        test_conf(&conf);
        ae_config_cleanup(&conf);

Note the `fflush`, which is important, otherwise, the file could be empty when
read by `ae_config_read`.

This test suite will allow us to prove that our implementation is functional.
To prove it's robustness, one should at least add limit cases tests, to test
things like: "what happens when an argument is NULL ?".

# The implementation - [ae_config.c]

Most of it is ridiculously simple.
First comes `ae_config_read_from_string`, which is one line long:

        return -argz_create_sep(string, '\n', &conf->argz, &conf->len);

`ae_config_get` is even shorter:

        return envz_get(conf->argz, conf->len, key);

We start doing some "serious" work, with `ae_config_cleanup`, which is ... two
lines long:

        free(conf->argz);
        memset(conf, 0, sizeof(*conf));

Now we already have a functional string map, but the original aim was to have
a configuration file format, so the `ae_config_read` is the function we're
really interested in.
It's implementation, stripped of the mandatory error checks, is as follows:

        f = fopen(path, "rbe");
        /* compute the size of the file */
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        /* be kind, rewind */
        fseek(f, 0, SEEK_SET);

        /* read all */
        string = calloc(size, 1);
        fread(string, 1, size, f);

        return ae_config_read_from_string(conf, string);

# Conclusion

Once compiled, run the test with:

        ./test_ae_config

Or even better with:

        valgrind ./test_ae_config

The expected output is:

        test_ae_config.c
        test_string
        test_conf
        test_file
        test_conf

and the exit status should be 0, with no reported error under valgrind.

In less than 200 lines of C, we've shown a simple key / value pairs
configuration file format.
This file format has some characteristics, one may consider as limitations.
For example, one may want to strip spaces before and after the key and the
value.
Or keys containing space are impossible.
But yet, this configuration file format is useful and I've already used similar
things in production code, with success.

# Implementation notes

1. I make an extensive use of the gcc attribute `__attribute__((cleanup(...)))`.
   This way, variable with a life span with mustn't be more than the current
   function, are guaranteed to be cleaned up at return, even in error code
   paths.
   Extra care must be taken though, to assure that they never contain garbage
   when the function returns, hence the initialization to `NULL`.
2. I use nearly always, negative `errno` values to report errors, it is simple,
   effective and allows to use the C error API easily (`strerror`...) without
   having to deal with multiple error constants sets.
3. No checks are performed on the API arguments, this can be ok for an internal
   API, but would be dangerous for an API meant to be public.

[ae_config.c]: /resources/code/argz-envz-simple-config/ae_config.c
[ae_config.h]: /resources/code/argz-envz-simple-config/ae_config.h
[test_ae_config.c]: /resources/code/argz-envz-simple-config/test_ae_config.c
[glibc]: https://www.gnu.org/software/libc/manual/html_mono/libc.html
[argz]: https://www.gnu.org/software/libc/manual/html_mono/libc.html#Argz-Functions
[envz]: https://www.gnu.org/software/libc/manual/html_mono/libc.html#Envz-Functions