#include "internal/pumautil.h"

#include "internal/valgrind.h"

#include <assert.h>

void* _getLastElement(struct pumaThreadList* list)
{
	VALGRIND_MAKE_MEM_DEFINED(list, sizeof(struct pumaThreadList));
	struct pumaNode* tail = list->tail;

	size_t lastElementIndex;
	bool found;
	void* ret = NULL;

	while(tail && tail->active)
	{
		lastElementIndex = pumaLastIndexOfValue(tail->freeMask, MASKNOTFREE, &found);

		if(!found)
		{
			if(list->numNodes > 0)
				--list->numNodes;

			tail->active = false;
			tail = tail->prev;

			if(tail != NULL)
				list->tail = tail;
		}
		else
		{
			ret = _getElement(tail, lastElementIndex);
			break;
		}
	}
	VALGRIND_MAKE_MEM_NOACCESS(list, sizeof(struct pumaThreadList));
	return ret;
}

size_t _getIndexOfElement(void* element)
{
	struct pumaNode* node = _getNodeForElement(element);

	return _getIndexOfElementOnNode(element, node);
}

size_t _getIndexOfElementOnNode(void* element, struct pumaNode* node)
{
	char* arrayStart = node->elementArray;

	size_t index = (size_t)((char*)element - arrayStart) / node->elementSize;

	return index;
}

void* _getElement(struct pumaNode* node, size_t i)
{
	char* arrayStart = node->elementArray;

	void* element = (i * node->elementSize + arrayStart);

	return element;
}

struct pumaNode* _getNodeForElement(void* element)
{
	struct pumaNode* node =
			(struct pumaNode*)((size_t)element &
			~((pumaPageSize * PUMA_NODEPAGES) - 1));

	return node;
}