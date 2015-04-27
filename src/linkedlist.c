#include "linkedlist.h"
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

struct puma_listElem
{
	void *data;
	struct puma_listElem *next;
	struct puma_listElem *prev;
};

void* puma_popHeadFromLinkedList(struct puma_linkedList* l)
{
	if(l->numElements == 0)
		return NULL;

	--l->numElements;

	struct puma_listElem *e = l->head->next;
	l->head->next = e->next;
	e->next->prev = l->head;

	void* ret = e->data;

	free(e);

	return ret;
}

void* puma_popTailFromLinkedList(struct puma_linkedList* l)
{
	if(l->numElements == 0)
		return NULL;

	--l->numElements;

	struct puma_listElem *e = l->tail->prev;
	l->tail->prev = e->prev;
	e->prev->next = l->tail;

	void* ret = e->data;

	free(e);

	return ret;
}

void puma_pushStartLinkedList(struct puma_linkedList* l, void* data)
{
	++l->numElements;

	struct puma_listElem *currentElement = l->head->next;

	struct puma_listElem *newElem = calloc(1, sizeof(struct puma_listElem));
	newElem->data = data;
	newElem->next = currentElement;
	newElem->prev = currentElement->prev;
	newElem->prev->next = newElem;
	currentElement->prev = newElem;
}

void puma_pushEndLinkedList(struct puma_linkedList* l, void* data)
{
	++l->numElements;

	struct puma_listElem *currentElement = l->tail;

	struct puma_listElem *newElem = calloc(1, sizeof(struct puma_listElem));
	newElem->data = data;
	newElem->next = currentElement;
	newElem->prev = currentElement->prev;
	newElem->prev->next = newElem;
	currentElement->prev = newElem;
}

bool puma_linkedlistIsOrdered(struct puma_linkedList* l, comparatorFunc comparator)
{
	struct puma_listElem *currentElement = l->head->next;
	struct puma_listElem *nextElement = currentElement->next;
	while(currentElement != l->tail && nextElement != l->tail)
	{
		if(comparator(currentElement, nextElement) == 1)
			return false;
		currentElement = nextElement;
		nextElement = nextElement->next;
	}

	return true;
}

void puma_insertIntoLinkedList(struct puma_linkedList* l, void* data, comparatorFunc comparator)
{
	++l->numElements;

	struct puma_listElem *currentElement = l->head->next;

	while(currentElement != l->tail &&
	 		(comparator != NULL ? comparator(currentElement->data, data) < 0 : false))
	{
		currentElement = currentElement->next;
	}

	struct puma_listElem *newElem = calloc(1, sizeof(struct puma_listElem));
	newElem->data = data;
	newElem->next = currentElement;
	newElem->prev = currentElement->prev;
	newElem->prev->next = newElem;
	currentElement->prev = newElem;
}

struct puma_linkedList* puma_createLinkedList()
{
	struct puma_linkedList *l = calloc(1, sizeof(struct puma_linkedList));
	puma_initLinkedList(l);
	return l;
}

void puma_initLinkedList(struct puma_linkedList* l)
{
	l->head = (struct puma_listElem*)calloc(1, sizeof(struct puma_listElem));
	l->tail = (struct puma_listElem*)calloc(1, sizeof(struct puma_listElem));
	l->head->next = l->tail;
	l->tail->prev = l->head;

	l->numElements = 0;
}

void puma_destroyLinkedList(struct puma_linkedList* l, dataDestructor destroy)
{
	void* data = puma_popHeadFromLinkedList(l);

	while(data)
	{
		if(destroy)
			destroy(data);
		data = puma_popHeadFromLinkedList(l);
	}

	if(l->head)
		free(l->head);
	if(l->tail)
		free(l->tail);
}

void puma_deleteLinkedList(struct puma_linkedList* l, dataDestructor destroy)
{
	puma_destroyLinkedList(l, destroy);
	free(l);
}