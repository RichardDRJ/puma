#include "pumalloc.h"
#include "internal/valgrind.h"

#include "internal/alloc.h"
#include "internal/free.h"
#include "internal/pumautil.h"
#include "internal/pumaheader.h"
#include "internal/pumanode.h"
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
	size_t index = set->splitter(balData, set->numThreads,
			set->splitterExtraData);
	size_t adjustedIndex = set->threadListToIndex[index];
	struct pumaThreadList* threadList = &set->threadLists[adjustedIndex];
	return _pumallocOnThreadList(threadList);
}

void* pumalloc(struct pumaSet* set)
{
	struct pumaThreadList* threadList = _getListForCurrentThread(set);
	return _pumallocOnThreadList(threadList);
}

void* pumallocAutoBalancing(struct pumaSet* set, int* allocatedThread)
{
	size_t minThread = 0;

	VALGRIND_MAKE_MEM_DEFINED(set->threadLists,
			sizeof(struct pumaThreadList) * set->numCores);
	for(size_t thread = 1; thread < set->numCores; ++thread)
	{
		struct pumaThreadList* tmpThreadList = set->threadLists + thread;
		if(tmpThreadList->active && tmpThreadList->numElements < set->threadLists[minThread].numElements)
			minThread = thread;
	}

	if(allocatedThread != NULL)
		*allocatedThread = set->threadLists[minThread].tid;

	VALGRIND_MAKE_MEM_NOACCESS(set->threadLists,
			sizeof(struct pumaThreadList) * set->numCores);

	return pumallocOnThread(set, minThread);
}

void* pumallocOnThread(struct pumaSet* set, size_t thread)
{
	struct pumaThreadList* threadList = set->threadLists + thread;

	return _pumallocOnThreadList(threadList);
}

void pufree(void* element)
{
	struct pumaNode* node = _getNodeForElement(element);
	_freeElementOnNode(element, node);
}
