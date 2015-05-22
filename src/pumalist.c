#include "internal/numa.h"
#include "pumalist.h"
#include "internal/valgrind.h"
#include "internal/pumalog.h"
#include "internal/pumanode.h"
#include "internal/pumaheader.h"
#include "internal/pumathreadlist.h"
#include "internal/pumadomain.h"
#include "internal/pumautil.h"
#include "internal/numa.h"
#include "pumathreadpool.h"

#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#include <string.h>

#include <errno.h>

#include <assert.h>

static void _setupThreadListsWorker(void* arg)
{
	struct pumaList* list = (struct pumaList*)arg;
	int currDomain = _getCurrentNumaDomain();
	struct pumaDomain* domain = &list->domains[currDomain];
	size_t cpuIndex = _getCurrentCPUIndexInDomain();
	struct pumaThreadList* tl = &domain->listsInDomain[cpuIndex];
	tl->active = true;
	tl->numaDomain = currDomain;
	tl->tid = pumaGetThreadNum();
	list->threadListToIndex[tl->tid] = (size_t)(tl - list->threadLists);
	tl->elementSize = list->elementSize;
}

void pumaListSetBalancer(struct pumaList* list, bool autoBalance,
		splitterFunc splitter, void* splitterExtraData)
{
	list->autoBalance = autoBalance;
	list->splitter = splitter;
	list->splitterExtraData = splitterExtraData;
}

struct pumaList* createPumaList(size_t elementSize, size_t numThreads,
		char* threadAffinity)
{
	struct pumaList* newList =
			(struct pumaList*)malloc(sizeof(struct pumaList));

	if(pumaPageSize == 0)
		pumaPageSize = (size_t)sysconf(_SC_PAGESIZE);

	size_t numCores = _getNumCPUs();

	newList->elementSize = elementSize;
	newList->threadLists = (struct pumaThreadList*)calloc(numCores, sizeof(struct pumaThreadList));
	newList->numCores = numCores;
	newList->numThreads = numThreads;
#ifdef STATIC_THREADPOOL
	setupThreadPool(numThreads, threadAffinity);
	newList->threadPool = getThreadPool();
#else
	newList->threadPool = createThreadPool(numThreads, threadAffinity);
#endif // STATIC_THREADPOOL
	newList->threadListToIndex = calloc(numThreads, sizeof(size_t));

	size_t numDomains = _getNumDomains();
	newList->numDomains = numDomains;
	newList->domains = (struct pumaDomain*)calloc(numDomains, sizeof(struct pumaDomain));
	newList->autoBalance = true;
	struct pumaThreadList* tl = newList->threadLists;

	for(size_t i = 0; i < numDomains; ++i)
	{
		newList->domains[i].listsInDomain = tl;
		newList->domains[i].numListsInDomain = _getNumCPUsInDomain(i);
		tl += newList->domains[i].numListsInDomain;
	}

	executeOnThreadPool(newList->threadPool, &_setupThreadListsWorker, (void*)newList);

	for(size_t i = 0; i < numCores; ++i)
		VALGRIND_MAKE_MEM_NOACCESS(newList->threadLists + i, sizeof(struct pumaThreadList));

	return newList;
}

static void _destroyThreadListWorker(void* arg)
{
	struct pumaList* list = (struct pumaList*)arg;
	size_t thread = pumaGetThreadNum();
	struct pumaNode* currentNode = list->threadLists[thread].head;

	while(currentNode != NULL)
	{
		destroyPumaBitmask(&currentNode->freeMask);

		struct pumaNode* nextNode = currentNode->next;
		nufree_aligned(currentNode, currentNode->blockSize);
		currentNode = nextNode;
	}
}

void destroyPumaList(struct pumaList* list)
{
	VALGRIND_MAKE_MEM_DEFINED(list->threadLists,
			list->numCores * sizeof(struct pumaThreadList));
	
	executeOnThreadPool(list->threadPool, &_destroyThreadListWorker, (void*)list);

	free(list->threadLists);
	free(list->threadListToIndex);
	free(list->domains);
	free(list);
}
