#ifndef _PUMA__INTERNAL__PUMA_UTIL_H_
#define _PUMA__INTERNAL__PUMA_UTIL_H_

#include "internal/pumanode.h"
#include "internal/pumaheader.h"
#include "internal/pumathreadlist.h"
#include "internal/valgrind.h"

#include <unistd.h>
#include <stdlib.h>

void* _getLastElement(struct pumaThreadList* list);
size_t _getIndexOfElement(void* element);
size_t _getIndexOfElementOnNode(void* element, struct pumaNode* node);
void* _getElement(struct pumaNode* node, size_t i);
struct pumaNode* _getNodeForElement(void* element);

extern size_t pumaPageSize;

static inline size_t _getSmallestContainingPages(const size_t size)
{
	size_t pageAligned = size & ~(pumaPageSize - 1);
	return pageAligned + (pageAligned != size) * pumaPageSize;
}

#endif // _PUMA__INTERNAL__PUMA_UTIL_H_
