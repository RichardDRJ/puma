#include "minunit.h"
#include <stdlib.h>
#include <string.h>
#include "pumalloc.h"
#include "internal/pumathreadlist.h"
#include "internal/pumautil.h"
#include "internal/numa.h"

extern void* _pumallocOnThreadList(struct pumaThreadList* threadList);
extern size_t pumaPageSize;

char* test_getLastElementGetsLastElement(void)
{
	for(size_t elementSize = 10; elementSize < 500; ++elementSize)
	{
		struct pumaThreadList list;
		memset(&list, 0, sizeof(struct pumaThreadList));
		list.active = true;
		list.tid = 0;
		list.elementSize = elementSize;

		void** elements = malloc(sizeof(void*) * 1000);

		for(size_t freeIndex = 0; freeIndex < 1000; ++freeIndex)
		{
			elements[freeIndex] = _pumallocOnThreadList(&list);
			mu_assert(elements[freeIndex] == _getLastElement(&list),
					"_getLastElement returns the wrong value!");
		}

		_emptyThreadList(&list);
		free(elements);
	}

	return NULL;
}

char* test_getIndexOfElementAndGetElementAreEquivalent(void)
{
	pumaPageSize = (size_t)sysconf(_SC_PAGESIZE);

	for(size_t elementSize = 10;
			elementSize < pumaPageSize * PUMA_NODEPAGES -
					sizeof(struct pumaNode) - sizeof(uint64_t);
			++elementSize)
	{
		struct pumaThreadList list;
		memset(&list, 0, sizeof(struct pumaThreadList));
		list.active = true;
		list.tid = 0;
		list.elementSize = elementSize;

		struct pumaNode* node = _appendPumaNode(&list, elementSize);

		for(size_t i = 0; i < node->capacity; ++i)
		{
			void* e = _getElement(node, i);
			mu_assert(i == _getIndexOfElement(e),
					"_getIndexOfElement returns the wrong value!");
		}

		_freePumaNode(node);
	}

	return NULL;
}
