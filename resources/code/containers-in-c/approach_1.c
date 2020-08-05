#include <stdlib.h>
#include <stdio.h>

struct my_datum {
	int a;
	char b;
	struct my_datum *next;
};

/* insertion, modifies the head, hence the double star */
static void my_datum_push(struct my_datum **head, struct my_datum *element)
{
	if (head == NULL || element == NULL)
		return;

	element->next = *head;
	*head = element;
}

/* removal, same comment */
static struct my_datum *my_datum_pop(struct my_datum **head)
{
	struct my_datum *res;

	res = *head;
	if (res != NULL)
		*head = res->next;

	return res;
}

/* next element - walkthrough */
static struct my_datum *my_datum_next(struct my_datum *previous)
{
	if (previous == NULL)
		return NULL;

	return previous->next;
}

int main(void)
{
	struct my_datum datum_a = {.a = 20, .b = 'a', .next = NULL};
	struct my_datum datum_b = {.a = 42, .b = 'b', .next = NULL};
	struct my_datum datum_c = {.a = 100, .b = 'c', .next = NULL};
	struct my_datum *head = NULL;
	struct my_datum *cursor;

	my_datum_push(&head, &datum_a);
	my_datum_push(&head, &datum_b);
	my_datum_push(&head, &datum_c);

	for (cursor = head; cursor != NULL; cursor = my_datum_next(cursor))
		printf("datum %c: a = %d\n", cursor->b, cursor->a);
	while (head != NULL)
		printf("popping datum %c\n", my_datum_pop(&head)->b);

	return EXIT_SUCCESS;
}
