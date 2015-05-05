#ifndef __DATA_STRUCTURES__LINKED_LIST_H__
#define __DATA_STRUCTURES__LINKED_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

typedef int (*comparatorFunc)(const void* a, const void* b);
typedef void (*dataDestructor)(void* d);

struct puma_listElem;

struct puma_linkedList
{
	struct puma_listElem *head;
	struct puma_listElem *tail;

	size_t numElements;
};

void* puma_popHeadFromLinkedList(struct puma_linkedList *l);
void* puma_popTailFromLinkedList(struct puma_linkedList* l);
void puma_pushStartLinkedList(struct puma_linkedList* l, void* data);
void puma_pushEndLinkedList(struct puma_linkedList* l, void* data);
void puma_insertIntoLinkedList(struct puma_linkedList *l, void *data, comparatorFunc comparator);
struct puma_linkedList* puma_createLinkedList();
void puma_initLinkedList(struct puma_linkedList* l);
void puma_destroyLinkedList(struct puma_linkedList* l, dataDestructor destroy);
void puma_deleteLinkedList(struct puma_linkedList* l, dataDestructor destroy);
bool puma_linkedlistIsOrdered(struct puma_linkedList* l, comparatorFunc comparator);

#ifdef __cplusplus
}
#endif

#endif // __DATA_STRUCTURES__LINKED_LIST_H__