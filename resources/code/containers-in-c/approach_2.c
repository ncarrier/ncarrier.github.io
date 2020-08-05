#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct my_datum {
	int a;
	char b;
};

struct node {
	struct node *next;
	void *content;
};

/* insertion, performs an allocation, hence the error return code */
static int node_push(struct node **head, void *element)
{
	struct node *node;

	if (head == NULL || element == NULL)
		return -EINVAL;

	node = calloc(1, sizeof(*node));
	if (node == NULL)
		return -errno;

	*node = (struct node) {
		.next = *head,
		.content = element,
	};
	*head = node;

	return 0;
}

/* removal, frees the containing node */
static void *node_pop(struct node **head)
{
	void *res;
	struct node *old_head;

	if (head == NULL || *head == NULL)
		return NULL;

	old_head = *head;
	*head = old_head->next;
	res = old_head->content;
	free(old_head);

	return res;
}

/* next element - walkthrough */
static struct node *node_next(struct node *previous)
{
	if (previous == NULL)
		return NULL;

	return previous->next;
}

static void *node_get_datum(struct node *node)
{
	if (node == NULL)
		return NULL;

	return node->content;
}

int main(void)
{
	struct my_datum datum_a = {.a = 20, .b = 'a'};
	struct my_datum datum_b = {.a = 42, .b = 'b'};
	struct my_datum datum_c = {.a = 100, .b = 'c'};
	struct node *head = NULL;
	struct node *cursor;
	struct my_datum *element;

	node_push(&head, &datum_a);
	node_push(&head, &datum_b);
	node_push(&head, &datum_c);

	for (cursor = head; cursor != NULL; cursor = node_next(cursor)) {
		element = node_get_datum(cursor);
		printf("datum %c: a = %d\n", element->b, element->a);
	}
	while (head != NULL) {
		element = node_pop(&head);
		printf("popping datum %c\n", element->b);
	}

	return EXIT_SUCCESS;
}
