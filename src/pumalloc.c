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
	tail->active = true;

	void* newElement = _getElement(tail, freeIndex);
	_allocElementOnNode(tail, newElement);

	VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
	
	return newElement;
}

void* pumallocManualBalancing(struct pumaList* list, void* balData)
{
	size_t index = list->splitter(balData, list->numThreads,
			list->splitterExtraData);
	size_t adjustedIndex = list->threadListToIndex[index];
	struct pumaThreadList* threadList = &list->threadLists[adjustedIndex];
	return _pumallocOnThreadList(threadList);
}

void* pumalloc(struct pumaList* list)
{
	struct pumaThreadList* threadList = _getListForCurrentThread(list);
	return _pumallocOnThreadList(threadList);
}

void* pumallocAutoBalancing(struct pumaList* list, int* allocatedThread)
{
	size_t minThread = 0;

	VALGRIND_MAKE_MEM_DEFINED(list->threadLists,
			sizeof(struct pumaThreadList) * list->numCores);
	for(size_t thread = 1; thread < list->numCores; ++thread)
	{
		struct pumaThreadList* tmpThreadList = list->threadLists + thread;
		if(tmpThreadList->active && tmpThreadList->numElements < list->threadLists[minThread].numElements)
			minThread = thread;
	}

	if(allocatedThread != NULL)
		*allocatedThread = list->threadLists[minThread].tid;

	VALGRIND_MAKE_MEM_NOACCESS(list->threadLists,
			sizeof(struct pumaThreadList) * list->numCores);

	return pumallocOnThread(list, minThread);
}

void* pumallocOnThread(struct pumaList* list, size_t thread)
{
	struct pumaThreadList* threadList = list->threadLists + thread;

	return _pumallocOnThreadList(threadList);
}

void pufree(void* element)
{
	struct pumaNode* node = _getNodeForElement(element);
	_freeElementOnNode(element, node);
}
