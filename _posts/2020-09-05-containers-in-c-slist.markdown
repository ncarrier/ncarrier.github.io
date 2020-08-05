---
layout: post
title:  "Generic containers in C - sys/queue.h's slist API"
date:   2020-09-05 07:28:00
categories: C glibc
longtitle: "A first detailed example of how sys/queue.h sub-APIs work"
author: Nicolas Carrier
---

We've seen in the [first part of this posts series][part_1] possible ways to
implement containers in C.  
Because most of the time, the best code is the one you didn't have to write by
yourself, in this article, we'll study more in detail **sys/queue.h**'s
singly-linked list API, of which we've only scratched the surface so far.

# Prerequisites

The code sample quoted in this post, can be found in my
[snippets github repository][snippets] repository.
A simple `Makefile` is provided for sources in `snippet` but a simple
`your_compiler snippet.c -o snippet` command will work too.
The only requirement is that the C library provides the `sys/queue.h` header,
or that it has been copied in some place accessible by the program.

# Usage

## Creation

For this API (and for all the other **sys/queue.h** sub-APIs), two structures
must be declared, one for the list itself and one for the entries it's composed
of.

The list head structure is declared this way:
{% highlight C %}
SLIST_HEAD(slist_head, char_node);
{% endhighlight %}

Which expands to:
{% highlight C %}
struct slist_head {
	struct char_node *slh_first;
}
{% endhighlight %}

An entry in the list is a custom data structure of which some of the fields are
declared using the `SLIST_ENTRY` macro, concretely, one can use it this way:
{% highlight C %}
struct char_node {
	SLIST_ENTRY(char_node) node;
	char data;
};
{% endhighlight %}

Which expands to:
{% highlight C %}
struct char_node {
	struct {
		struct char_node *sle_next;
	} node;
	char data;
};
{% endhighlight %}

## Initialization

Once the structures are declared, we have to initialize them.
For `struct char_node`, nothing particular, the `node.sle_next` field doesn't
have to be initialized as it will be done by the slist's API, so we just have to
initialize our custom fields, `.data` [in this example][char_node_init].

On the contrary, the `struct slist_head` structure has to be properly
initialized, for that, two facilities are provided, the first one is an
initializer macro, `SLIST_HEAD_INITIALIZER`, which can be used either this way:
{% highlight C %}
	struct slist_head head = SLIST_HEAD_INITIALIZER(head);
{% endhighlight %}

Or [this way][slist_head_init]:
{% highlight C %}
	struct slist_head head;

	head = (struct slist_head) SLIST_HEAD_INITIALIZER(head);
{% endhighlight %}

The [second possibility][slist_head_init_2] is to use the `SLIST_INIT`
function-like macro, which will have the same effect:

{% highlight C %}
	struct slist_head head;

	SLIST_INIT(&head);
{% endhighlight %}

## Insertions

It's possible to insert either at the head of the slist, or anywhere else after
one of the lists' elements.

Insertion at the head is done using `SLIST_INSERT_HEAD`
[this way][head_insertion]:
{% highlight C %}
SLIST_INSERT_HEAD(&head, &node_a, node);
{% endhighlight %}

Insertion after an element of the list is done using `SLIST_INSERT_AFTER`
[this way][after_insertion]:
{% highlight C %}
SLIST_INSERT_AFTER(&node_a, &node_c, node);
{% endhighlight %}
This will insert `node_c` after `node_a` without needing a reference on the
list's head.

## Walk through

The `SLIST_FOREACH` macro is provided for iterating through the list, it can be
used [this way][walk_through]:

{% highlight C %}
SLIST_FOREACH(cur, &head, node)
	printf("\tvalue is "CHAR_NODE_FMT"\n", cur->data, cur->data);
{% endhighlight %}

It expands to:

{% highlight C %}
for((cur) = (&head)->slh_first; (cur); (cur) = (cur)->node.sle_next)
	printf("\tvalue is "CHAR_NODE_FMT"\n", cur->data, cur->data);
{% endhighlight %}

Which of course, requires that `cur` is defined beforehand this way:

{% highlight C %}
struct char_node *cur;
{% endhighlight %}

## Removal

Use `SLIST_REMOVE` [this way][removal]:

{% highlight C %}
SLIST_REMOVE(&head, &node_b, char_node, node);
{% endhighlight %}

Of course, be careful when removing elements from a list which you're
iterating on 😅.

`SLIST_REMOVE_HEAD` can be used too in order to remove the element at the head,
but this situation is already handled correctly by `SLIST_REMOVE`.

# Other operations

* [Getting the first][first] element with `SLIST_FIRST`:

{% highlight C %}
first = SLIST_FIRST(&head);
{% endhighlight %}

* [Getting the next][next] element, for example when wanting to iterate manually:

{% highlight C %}
second = SLIST_NEXT(first, node);
{% endhighlight %}

* Testing if [a list is empty][empty]:

{% highlight C %}
SLIST_EMPTY(&head)
{% endhighlight %}

# Last words

Not much more here to say, the API works as intended and as a bonus, it comes
with an extensive documentation accessible on my machine, with `man queue`
and also available online, for example [here][man].
In it, you will find detailed information on how they work, their performances
and impact on code size and the extra operations each implementation adds when
compared to others.

Similarly, other API present in the glibc offer ready to use implementations of
hash maps (`man hsearch`), binary search trees (`man tsearch`), as well as
pre-implemented search (`man lsearch` and `man bsearch`) or sorting algorithms
(`man qsort`).
Those subjects will be discussed in future articles.


[part_1]: /c/glibc/2019/05/01/containers-in-c-implementation.html
[snippets]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c
[char_node_init]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L18
[slist_head_init]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L29
[slist_head_init_2]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L31
[head_insertion]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L34
[after_insertion]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L36
[walk_through]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L42
[removal]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L46
[first]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L53
[next]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L56
[empty]: https://github.com/ncarrier/snippets/blob/1b4ceb3eed3ea21dfaf285e6130ec1712d458a11/queue/slist.c#L62
[man]: https://linux.die.net/man/3/queue