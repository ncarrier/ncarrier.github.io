#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
/* for offsetof, used in container_of */
#include <stddef.h>

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

struct node {
	struct node *next;
};

struct my_datum {
	int a;
	char b;
	/* notes: this is not a pointer and the field can be anywhere */
	struct node node;
};

/* insertion */
static void node_push(struct node **head, struct node *node)
{
	if (head == NULL || node == NULL)
		return;

	node->next = *head;
	*head = node;
}

/* removal */
static struct node *node_pop(struct node **head)
{
	struct node *old_head;

	if (head == NULL || *head == NULL)
		return NULL;

	old_head = *head;
	*head = old_head->next;

	return old_head;
}

/* next element - walkthrough */
static struct node *node_next(struct node *previous)
{
	if (previous == NULL)
		return NULL;

	return previous->next;
}

#define node_get_datum(p) container_of((p), struct my_datum, node)

int main(void)
{
	struct my_datum datum_a = {.a = 20, .b = 'a', .node = {.next = NULL,}};
	struct my_datum datum_b = {.a = 42, .b = 'b', .node = {.next = NULL,}};
	struct my_datum datum_c = {.a = 100, .b = 'c', .node = {.next = NULL,}};
	struct node *head = NULL;
	struct node *cursor;
	struct my_datum *element;

	node_push(&head, &datum_a.node);
	node_push(&head, &datum_b.node);
	node_push(&head, &datum_c.node);

	for (cursor = head; cursor != NULL; cursor = node_next(cursor)) {
		element = node_get_datum(cursor);
		printf("datum %c: a = %d\n", element->b, element->a);
	}
	while (head != NULL) {
		element = node_get_datum(node_pop(&head));
		printf("popping datum %c\n", element->b);
	}

	return EXIT_SUCCESS;
}
