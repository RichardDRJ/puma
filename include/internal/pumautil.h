#ifndef _PUMALIST__INTERNAL__PUMA_UTIL_H_
#define _PUMALIST__INTERNAL__PUMA_UTIL_H_

#include "internal/pumanode.h"
#include "internal/pumaheader.h"
#include "internal/pumathreadlist.h"
#include "internal/valgrind.h"

#include <unistd.h>
#include <stdlib.h>

void* _getLastElement(struct pumaThreadList* list);
size_t _getIndexOfElement(void* element);
size_t _getIndexOfElementOnNode(void* element, struct pumaNode* node);

static inline void* _getElement(struct pumaNode* node, size_t i)
{
	char* arrayStart = node->elementArray;

	void* element = (i * node->elementSize + arrayStart);

	return element;
}

static inline struct pumaNode* _getNodeForElement(void* element)
{
	struct pumaNode* node =
			(struct pumaNode*)((size_t)element &
			~((pumaPageSize * PUMA_NODEPAGES) - 1));

	return node;
}

#endif // _PUMALIST__INTERNAL__PUMA_UTIL_H_
