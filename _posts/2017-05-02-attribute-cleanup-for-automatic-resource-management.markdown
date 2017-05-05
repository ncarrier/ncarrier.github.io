---
layout: post
title:  "__attribute((cleanup([...]))) for automatic resource management"
date:   2017-05-02 07:40:00
categories: C 
longtitle: "Or how to un-clutter your error management code"
author: Nicolas Carrier
---

For a C developer, proper handling of resources lifecycle, like freing allocated
memory, or closing files properly is a vital task, but which can prove to be
tedious depending on the situation.
This article will cover briefly one of the techniques, that GCC (and clang)
makes easy to implement, by mean of the `cleanup` [variable attribute][doc].

In this blog post, I'll explore different scenarios, making use of this
attribute, and I'll talk about some of the pitfalls to avoid.

# Prerequisite

To build the code listed in this blog post, you'll only need a working version
of `gcc`, no special dependency is required.
Having a working `valgrind` installed will be handy when checking for memory
leaks.

All source codes can be compiled by simply executing (adapt to the source file
you want to compile):

	gcc -O0 -g3 -Werror -Wall -Wextra string_with_cleanup.c -o string_with_cleanup

Then testing can be done with:

	valgrind ./string_with_cleanup

If everything went well, you should have as many frees as allocs.

# Dive in

Let's start with a simple example, suppose you're writing a function, which must
work on a buffer wich you don't want to modify, for example, for future reuse.
A solution to do this is to duplicate the buffer, for example with `calloc` +
`memcpy`.
In this situation, you have to manage the lifecycle of the duplicated buffer:
you'll want to free it at the function's successful exit, but also at each early
return, due, for example, to an error.

This is shown in the following code, extracted from [string_without_cleanup.c]:

{% highlight C %}
/*
 * takes a frobnicated buffer containing a c-string, un-frobnicates it and
 * converts the result to an integer
 */
static int parse_frobnicated_int(const char *src, size_t size,
		int *retval)
{
	char *string;
	long value;
	char *endptr;

	string = calloc(size, sizeof(*string));
	if (string == NULL)
		return -errno;
	memcpy(string, src, size);

	if ((string[size - 1] ^ 42) != '\0') {
		cleanup_string(&string);
		return -EINVAL;
	}

	memfrob(string, size);
	value = strtol(string, &endptr, 0);
	if (*endptr != '\0') {
		cleanup_string(&string);
		return -EINVAL;
	}
	*retval = value;

	cleanup_string(&string);

	return 0;
}
{% endhighlight %}

In our example, a string buffer has been "frobnicated" by [memfrob], that is,
each of it's bytes, including the ending `'\0'` has been xor-ed with the value
`42`.
The `parse_frobnicated_int()` function un-"frobnicates" it and converts it's
original value to an integer.

Here, there are 3 points where the functions exits, past the `calloc`.
Now imagine when this function grows bigger, with more allocations which must
stay local to the function.
Now imagine you want to refactor the code and the order of the lines changes.
You have to go through all the function's body and check that each output point
past the allocations has the required freing code and not more.

One solution can be to force your function to have one and only one exit point,
with a label and some gotos.
While this solution works perfectly well, `__attribute__((cleanup(...)))` is,
in my humble opinion, a more elegant one.

Using it, the `parse_frobnicated_int` function can become (see
[string_with_cleanup.c]):

{% highlight C %}
static int parse_frobnicated_int(const char *src, size_t size,
		int *retval)
{
	char __attribute__((cleanup(cleanup_string)))*string = NULL;
	long value;
	char *endptr;

	string = calloc(size, sizeof(*string));
	if (string == NULL)
		return -errno;
	memcpy(string, src, size);

	if ((string[size - 1] ^ 42) != '\0')
		return -EINVAL;

	memfrob(string, size);
	value = strtol(string, &endptr, 0);
	if (*endptr != '\0')
		return -EINVAL;
	*retval = value;

	return 0;
}
{% endhighlight %}

When the function returns, the cleanup function for each variable marked with
the *cleanup* attribute is called, in reverse order, hence `string` is
guaranteed to be freed for every code path possible.

There are multiple gains with this rewrite :

* the cleanup code is written at *only one location*: the variables declaration.
* it's clearly stated *at the moment the variable is declared*, that it is local
(not returned to the caller)
* the only maintenance work, while refactoring, is to *match the order* of the
declarations with that of the corresponding allocations
* and as a bonus, a small amount of lines of code have been saved

# Caveats

## Initialize and check it when cleaning up

Suppose you want to check that the arguments of `parse_frobnicated_int` are
valid.
One may want to add something like:

{% highlight C %}
	if (src == NULL || *src == '\0' || retval == NULL)
		return -EINVAL;
{% endhighlight %}

Which is a good idea in itself.
In this situation, the cleanup function is still called, even if the allocation
has not been performed.
But this is not a problem since, `string` has been initialized to `NULL` and
because `cleanup_string()` does nothing if it's argument points to `NULL`.

A general rule of thumb is to always initialize at declaration, the variables
you want to cleanup, to a default value the cleanup function can interpret as
saying: "don't perform any cleanup".

## The case of asprintf

GNU libc's `asprintf` function family is really useful, for example, when
building a path "on the fly", to open a file.
It works like `snprintf` but allocates a buffer of the right size.
Coupled with `__attribute__((cleanup(...)))` it makes string creation painless.
But as emphasized in the [man page][asprintf], the GNU version offers no
guarantee over the content of the string pointer on error.

Consider the following code, excerpt of [asprintf.c]:

{% highlight C %}
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
{% endhighlight %}

Note the `path = NULL;` line, as `asprintf` offers no guarantee on the content
of path in case of error, we have to make sure that it is set to `NULL` before
`cleanup_string` is called, at the following `return NULL;` line.
I had to choose arbitrarily to put ENOMEM into `errno`, because the man page
give no guarantee either, on it's value.
It's a pity that GNU developers didn't choose to avoid these two problems with a
stricter API contract.

The rule this code illustrates can be seen as a special case of the previous
one: the developer has to guarantee that the cleanup function has enough
information to know whether or not it has to perform a cleanup.

# Going further

This coding technique can be used in many situations, let's list some:

 * automatic cleanup of file descriptors or file streams
 * automatic cleanup or destruction of custom structures
 * automatic mutex (or lock in general) release

The following pattern, for automatic lock management is quite handy:

{% highlight C %}
void mystruct_lock(struct mystruct *s);

static void mystruct_unlock(struct mystruct **s)
{
	if (s == NULL || *s == NULL)
		return;
	/* cleanup */
	...
}

int foo(void) {
	struct mystruct __attribute__(cleanup((mystruct_unlock))) *s = NULL;

	...

	ret = mylock_lock(s = /* address of the object to lock */);
	if (ret < 0) {
		/* handle error */
		...
	}
	...
{% endhighlight %}

This way, automatic cleanup of the unlocking is programmed *at the moment the
lock is acquired*, which makes it less likely to be done at the wrong place.

# Last words

The `__attribute__((cleanup(...)))` attribute is a tools which allows to
implement what some may call [RAII] and is quite simple to use, provided the
following rules are respected:

 * guarantee the cleanup function knows what it has to cleanup
 * match the order of the allocations with the corresponding declarations

[doc]: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html
[memfrob]: https://linux.die.net/man/3/memfrob
[string_without_cleanup.c]: /resources/code/attribute-cleanup-for-automatic-resource-management/string_without_cleanup.c
[string_with_cleanup.c]: /resources/code/attribute-cleanup-for-automatic-resource-management/string_with_cleanup.c
[asprintf]: https://linux.die.net/man/3/asprintf
[asprintf.c]: /resources/code/attribute-cleanup-for-automatic-resource-management/asprintf.c
[RAII]: https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization