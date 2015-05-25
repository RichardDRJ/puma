#include "internal/numa.h"
#include "pumaset.h"
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
	struct pumaSet* set = (struct pumaSet*)arg;
	int currDomain = _getCurrentNumaDomain();
	struct pumaDomain* domain = &set->domains[currDomain];
	size_t cpuIndex = _getCurrentCPUIndexInDomain();
	struct pumaThreadList* tl = &domain->listsInDomain[cpuIndex];
	tl->active = true;
	tl->numaDomain = currDomain;
	tl->tid = pumaGetThreadNum();
	set->threadListToIndex[tl->tid] = (size_t)(tl - set->threadLists);
	tl->elementSize = set->elementSize;
}

void pumaListSetBalancer(struct pumaSet* set, bool autoBalance,
		splitterFunc splitter, void* splitterExtraData)
{
	set->autoBalance = autoBalance;
	set->splitter = splitter;
	set->splitterExtraData = splitterExtraData;
}

struct pumaSet* createPumaSet(size_t elementSize, size_t numThreads,
		char* threadAffinity)
{
	struct pumaSet* newList =
			(struct pumaSet*)malloc(sizeof(struct pumaSet));

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

struct _getNumElementsArg
{
	struct pumaSet* set;
	size_t* sums;
};

static void _getNumElementsWorker(void* arg)
{
	struct _getNumElementsArg* vars = (struct _getNumElementsArg*)arg;
	struct pumaSet* set = vars->set;
	size_t thread = pumaGetThreadNum();

	int currDomain = _getCurrentNumaDomain();
	struct pumaDomain* domain = set->domains + currDomain;
	struct pumaThreadList* listsInDomain = domain->listsInDomain;
	size_t numListsInDomain = domain->numListsInDomain;
	VALGRIND_MAKE_MEM_DEFINED(listsInDomain,
			numListsInDomain * sizeof(struct pumaThreadList));

	struct pumaThreadList* threadList =
			listsInDomain + _getCurrentCPUIndexInDomain();

	VALGRIND_MAKE_MEM_NOACCESS(listsInDomain,
			numListsInDomain * sizeof(struct pumaThreadList));

	VALGRIND_MAKE_MEM_DEFINED(threadList, sizeof(struct pumaThreadList));
	vars->sums[thread] += threadList->numElements;
	VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
}

size_t getNumElements(struct pumaSet* set)
{
	size_t numThreads = pumaGetNumThreads(set->threadPool);
	size_t sums[numThreads];
	memset(sums, 0, numThreads * sizeof(size_t));

	struct _getNumElementsArg arg;
	arg.set = set;
	arg.sums = sums;

	executeOnThreadPool(set->threadPool, &_getNumElementsWorker, (void*)(&arg));

	size_t sum = 0;

	for(size_t i = 0; i < numThreads; ++i)
		sum += sums[i];

	return sum;
}

static void _destroyThreadListWorker(void* arg)
{
	struct pumaSet* set = (struct pumaSet*)arg;
	size_t thread = pumaGetThreadNum();
	struct pumaNode* currentNode = set->threadLists[thread].head;

	while(currentNode != NULL)
	{
		destroyPumaBitmask(&currentNode->freeMask);

		struct pumaNode* nextNode = currentNode->next;
		nufree_aligned(currentNode, currentNode->blockSize);
		currentNode = nextNode;
	}
}

void destroyPumaSet(struct pumaSet* set)
{
	VALGRIND_MAKE_MEM_DEFINED(set->threadLists,
			set->numCores * sizeof(struct pumaThreadList));
	
	executeOnThreadPool(set->threadPool, &_destroyThreadListWorker, (void*)set);

	free(set->threadLists);
	free(set->threadListToIndex);
	free(set->domains);
	free(set);
}
