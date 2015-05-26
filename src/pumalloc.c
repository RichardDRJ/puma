#include "pumalloc.h"
#include "internal/valgrind.h"

#include "internal/alloc.h"
#include "internal/free.h"
#include "internal/pumautil.h"
#include "internal/pumaheader.h"
#include "internal/pumanode.h"
#include "internal/pumadomain.h"
#include "internal/pumathreadlist.h"


void* _pumallocOnThreadList(struct pumaThreadList* threadList)
{
	VALGRIND_MAKE_MEM_DEFINED(threadList, sizeof(struct pumaThreadList));
	struct pumaNode* tail = threadList->tail;

	if(tail == NULL || tail->numElements == tail->capacity)
		tail = _appendPumaNode(threadList, threadList->elementSize);

	size_t freeIndex = tail->firstKnownFree++;

	if(freeIndex >= tail->capacity)
	{
		_cleanupNode(tail, threadList);
		freeIndex = tail->numElements;
		tail->firstKnownFree = freeIndex + 1;
	}

	threadList->numNodes += (!tail->active);
	threadList->tail = tail;
	tail->active = true;

	void* newElement = _getElement(tail, freeIndex);
	_allocElementOnNode(tail, newElement);

	VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
	
	return newElement;
}

void* pumallocManualBalancing(struct pumaSet* set, void* balData)
{
	size_t tid = set->splitter(balData, set->numThreads,
			set->splitterExtraData);
	struct pumaThreadList* threadList = set->tidToThreadList[tid];
	return _pumallocOnThreadList(threadList);
}

void* pumalloc(struct pumaSet* set)
{
	struct pumaThreadList* threadList = _getListForCurrentThread(set);
	return _pumallocOnThreadList(threadList);
}

void* pumallocAutoBalancing(struct pumaSet* set)
{
	struct pumaThreadList* minThreadList = NULL;


	for(size_t d = 0; d < set->numDomains; ++d)
	{
		struct pumaDomain* domain = &set->domains[d];

		VALGRIND_MAKE_MEM_DEFINED(domain->listsInDomain,
				sizeof(struct pumaThreadList) * domain->numListsInDomain);
		for(size_t thread = 0; thread < domain->numListsInDomain; ++thread)
		{
			struct pumaThreadList* tmpThreadList = domain->listsInDomain + thread;
			if(tmpThreadList->active && minThreadList != NULL &&
					tmpThreadList->numElements < minThreadList->numElements)
			{
				minThreadList = tmpThreadList;
			}
		}

		VALGRIND_MAKE_MEM_NOACCESS(domain->listsInDomain,
				sizeof(struct pumaThreadList) * domain->numListsInDomain);
	}

	return _pumallocOnThreadList(minThreadList);
}

void pufree(void* element)
{
	struct pumaNode* node = _getNodeForElement(element);
	_freeElementOnNode(element, node);
}
