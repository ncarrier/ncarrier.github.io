#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/queue.h>

SLIST_HEAD(my_datum_slist, my_datum);

struct my_datum {
	int a;
	char b;
	SLIST_ENTRY(my_datum) node;
};

/* removal */
#define node_pop(head, field) ({\
	typeof((head)->slh_first) __res = SLIST_FIRST((head)); \
	SLIST_REMOVE_HEAD((head), field); \
	__res;})

int main(void)
{
	struct my_datum datum_a = {.a = 20, .b = 'a', .node = {.sle_next = NULL,}};
	struct my_datum datum_b = {.a = 42, .b = 'b', .node = {.sle_next = NULL,}};
	struct my_datum datum_c = {.a = 100, .b = 'c', .node = {.sle_next = NULL,}};
	struct my_datum_slist head = SLIST_HEAD_INITIALIZER(head);
	struct my_datum *element;

	SLIST_INSERT_HEAD(&head, &datum_a, node);
	SLIST_INSERT_HEAD(&head, &datum_b, node);
	SLIST_INSERT_HEAD(&head, &datum_c, node);

	for (element = SLIST_FIRST(&head);
			element != NULL;
			element = SLIST_NEXT(element, node)) {
		printf("datum %c: a = %d\n", element->b, element->a);
	}
	while (!SLIST_EMPTY(&head)) {
		element = node_pop(&head, node);
		printf("popping datum %c\n", element->b);
	}

	return EXIT_SUCCESS;
}
