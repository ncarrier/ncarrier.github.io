---
layout: post
title:  "Generic containers in C - Implementation"
date:   2020-08-05 07:28:00
categories: C glibc
longtitle: "Or how to avoid re-implementing broken linked lists"
author: Nicolas Carrier
---

Every non-trivial code, even C code, has to deal with data containers.
While arrays are sufficient in lots of cases, memory usage or algorithmic
complexity constraints may impose more clever choices.
The lack of containers in C standard library is a classical reproach made to the
C language, however, many container implementations are already shipped with the
glibc, making them, if not standard, at least widely available.  
This post is starting a new series, aiming at presenting the container
implementations available, show them at work and discuss of some pros and cons
of using them.

Since container implementation is a classic of computer science courses, most
developers do the mistake of rolling their own (which I did), although it costs
time and leads frequently, to painful bugs crawling in the code base.

In this article, we'll evaluate various implementation approaches for
containers, ending with the one use by one of **sys/queue.h**'s sub-APIs:
**slist**.

# Prerequisite

I suggest to compile source code examples provided with this article this way:
{% highlight bash %}
export CFLAGS="-Wall -Wextra -Wformat=2 -Wunused-variable \
    -Wold-style-definition -Wstrict-prototypes -Wno-unused-parameter \
    -Wmissing-declarations -Wmissing-prototypes -Wpointer-arith \
    -Werror -g3 -O0"
{% endhighlight %}

To be done only once, then for example:
{% highlight bash %}
make approach_1
{% endhighlight %}

# Container implementation in C

Most containers have to attach metadata to the data elements they contain.
This metadata can be for example, a pointer to a next element (linked list...),
a color (red-black trees...) and must be stored "along" with the data of
interest.
Let's explore a simple stack API implementation on top of a singly-linked list
container.

## First approach - add container fields to the data structure

Please see the example implementation [here][approach_1].

If we have, say, this kind of data structure, we want to embed in a
singly-linked list:

{% highlight C %}
struct my_datum {
	int a;
	char b;
};
{% endhighlight %}

A first approach is to add for example, a pointer to the next element in the
list:

{% highlight C %}
struct my_datum {
	int a;
	char b;
	struct my_datum *next;
};
{% endhighlight %}

By doing this, the list head can be a simple pointer to a `my_datum` element and
the empty list, the `NULL` pointer.
A possible list API can be:

{% highlight C %}
/* insertion, modifies the head, hence the double star */
void my_datum_push(struct my_datum **head, struct my_datum *element);
/* removal, same comment */
struct my_datum *my_datum_pop(struct my_datum **head);
/* next element - walkthrough */
struct my_datum *my_datum_next(struct my_datum *previous);
{% endhighlight %}

*Advantages*:

 * the node and the data it contains are allocated at the same time
 * type safety

*Problems*:

 * the API has to modify the original data structure
 * the data structure has to know which containers may contain it
 * the data structure has to embed one node-like structure for each container it
   may be stored into
 * the API is strongly coupled with the datum type

*Problem* : The coupling issue is the most annoying, when the need to chain
other data types will arise, all the function will have to be reimplemented or
copy-pasted and there is no possiblilities to unit test the linked-list API
separately.

## Second approach - genericity using `void *`

Please see the example implementation [here][approach_2].

Extracting the container's API can be done by storing the data as a `void *`
inside the containers metadata element.
This way, the data structure return to:

{% highlight C %}
struct my_datum {
	int a;
	char b;
};
{% endhighlight %}

And we need a new structure, say `node` capable of holding both the metadata and
a reference on the data:

{% highlight C %}
struct node {
	struct node *next;
	void *content;
};
{% endhighlight %}

The API can become now:

{% highlight C %}
/* insertion, performs an allocation, hence the error return code */
int node_push(struct node **head, void *element);
/* removal, frees the containing node */
void *node_pop(struct node **head);
/* next element - walkthrough */
struct node *node_next(struct node *previous);
/* datum accessor, required at least for a walkthrough */
void *node_get_datum(struct node *node);
{% endhighlight %}

*Advantages*:

 * now the API can be easily reused (and tested separately)
 * the data structure has no knowledge that it can be stored inside a container
 * the API doesn't modify the original data structure anymore.
 * a same datum can be part of multiple containers with no modifications

*Problems*:

 * type safety is lost
 * the node and the data it contains are now allocated separately

Another aspect to consider is that the types of data stored can now be
heterogeneous, but because there is no way to retrieve the original type of the
data stored, this should be avoided.

## Third approach - embed a container node inside the data structure

Please see the example implementation [here][approach_3].

A `node` structure can be defined, holding this time no reference to any data:

{% highlight C %}
struct node {
	struct node *next;
};
{% endhighlight %}

The original data structure now becomes:

{% highlight C %}
struct my_datum {
	int a;
	char b;
	/* notes: this is not a pointer and the field can be anywhere */
	struct node node;
};
{% endhighlight %}

Now the question is: "how to implement a generic API with these structures?",
for that, we'll use the `container_of` macro.

Ubiquitous in the kernel, the `container_of` macro allows, knowing the address
of a structure instance's `node` field, to retrieve the address of the structure
containing the field.
Its definition can be found, for example, in the
[linux kernel source code][container_of].
That is, if we have a `ptr` `struct node *` pointer, then
`container_of(ptr, struct my_datum, node)` gives the address of the
`struct my_datum` object whose `node` field address is `ptr`.

With this tool, we can design a manipulation API having knowledge only of the
`node` structure:

{% highlight C %}
/* insertion */
void node_push(struct node **head, struct node *node);
/* removal */
struct node *node_pop(struct node **head);
/* next element - walkthrough */
struct node *node_next(struct node *previous);
{% endhighlight %}

`node_get_datum` is usually defined as a macro (and would be named `to_my_datum`
if in the kernel) and can then be defined this way:

{% highlight C %}
#define node_get_datum(p) container_of((p), struct my_datum, node)
{% endhighlight %}

to retrieve a `my_datum` structure knowing a corresponding `struct node`
address.

*Advantages*:

 * the API still can be easily reused and tested separately
 * the node and the data it contains are allocated at the same time
 
*Problems*:

 * the API has to modify the original data structure
 * the data structure has to know which containers may contain it
 * the data structure has to embed one node-like structure for each container it
   may be stored into
 * there still is no type safety

## Fourth approach - the sys/queue.h API, or macros everywhere

Please see the example implementation [here][approach_4].

We'll use the slist API provided by **sys/queue.h**, which implements a
singly-linked list API.
The datum structure uses macros for defining the chaining structure elements.

First is the datum structure, defined this way:
{% highlight C %}
struct my_datum {
    int a;
    char b;
    SLIST_ENTRY(my_datum) node;
};
{% endhighlight %}

Which expands to:
{% highlight C %}
struct my_datum {
    int a;
    char b;
    struct {
        struct my_datum *sle_next;
    } node;
};
{% endhighlight %}

An additional structure is defined, to represent the head of the list, which can
be declared this way:
{% highlight C %}
SLIST_HEAD(my_datum_slist, my_datum);
{% endhighlight %}

Which expands to:
{% highlight C %}
struct my_datum_slist {
	struct my_datum *slh_first;
}
{% endhighlight %}

A complete API is provided, `node_push`'s equivalent is defined this way:
{% highlight C %}
#define	SLIST_INSERT_HEAD(head, elm, field) do {			\
	(elm)->field.sle_next = (head)->slh_first;			\
	(head)->slh_first = (elm);					\
} while (/*CONSTCOND*/0)
{% endhighlight %}

And is used this way:
{% highlight C %}
struct my_datum node_a = { .data = 'a' };
SLIST_HEAD(my_datum_slist, my_datum) head = SLIST_HEAD_INITIALIZER(head);

SLIST_INSERT_HEAD(&head, &node_a, node);
{% endhighlight %}

`node_pop` isn't provided but can be simulated by `SLIST_FIRST(head)` followed
by `SLIST_REMOVE_HEAD(head, field)`, for example:
{% highlight C %}
#define node_pop(head, field) ({\
    typeof((head)->slh_first) __res = SLIST_FIRST((head)) \
    SLIST_REMOVE_HEAD((head), field); \
    __res;})
{% endhighlight %}

And last, `node_next` functionality is provided by `SLIST_NEXT`.

Since the list type is not the same as the node type, some extra utilities have
to be provided, like `SLIST_FIRST`, which returns the first element of an slist
and `SLIST_EMPTY`, used to test whether the slist is empty or not.

*Advantages*:

 * the API still can be easily reused and tested separately
 * the node and the data it contains are allocated at the same time
 * type safety
 * it already exists and is available everywhere (one header file which can be
 copied as-is)

*Problems*:

 * the API has to modify the original data structure
 * the data structure has to know which containers may contain it
 * the data structure has to embed one node-like structure for each container it
   may be stored into
 * two data structures have to be defined instead of only once
 * using macros everywhere is complex to understand and hides things

# Last words

Choosing which approach is the best fit isn't a matter of counting the pros and
cons, because some will have more weight than others.
For my own part, reusing an already existing implementation without introducing
potentially heavy dependencies is the most important, that's why I'm using
**sys/queue.h** more and more in my development projects.

In my humble opinion, not having to deal with multiple allocations for one datum
node is the second most important aspect, think about when one has to chain some
data in 3 different lists.
That would lead to 4 objects' lifecycle to handle for each node of the list.
This aspect disqualifies the `void *` approach.

And to me, the third most important aspect, if forced to implement my own linked
list API is reuse and testability which disqualifies the first approach.

To conclude this article, when I'm forced to reimplement a linked list API, I
personnally use the `container_of` approach and otherwise, I use what the C
library already offers and which we'll see in more details in the next articles
of this series, starting with [a detailed usage example of slists][part_2].

[container_of]: https://elixir.free-electrons.com/linux/v3.2/source/include/linux/kernel.h#L659
[approach_1]: /resources/code/containers-in-c/approach_1.c
[approach_2]: /resources/code/containers-in-c/approach_2.c
[approach_3]: /resources/code/containers-in-c/approach_3.c
[approach_4]: /resources/code/containers-in-c/approach_4.c
[part_2]: /c/glibc/2020/09/05/containers-in-c-slist.html
