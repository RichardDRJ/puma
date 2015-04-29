#include "internal/numa.h"
#include "pumalist.h"
#include "internal/valgrind.h"
#include "internal/pumalog.h"
#include "internal/pumanode.h"
#include "internal/pumaheader.h"
#include "internal/pumathreadlist.h"
#include "internal/pumadomain.h"
#include "internal/pumautil.h"
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
	tl->elementSize = list->elementSize;
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
	newList->threadPool = newThreadPool(numThreads, threadAffinity);

	size_t numDomains = _getNumDomains();
	newList->numDomains = numDomains;
	newList->domains = (struct pumaDomain*)calloc(numDomains, sizeof(struct pumaDomain));
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
	struct pumaList* list;
	size_t* sums;
};

static void _getNumElementsWorker(void* arg)
{
	struct _getNumElementsArg* vars = (struct _getNumElementsArg*)arg;
	struct pumaList* list = vars->list;
	size_t thread = pumaGetThreadNum();

	int currDomain = _getCurrentNumaDomain();
	struct pumaDomain* domain = list->domains + currDomain;
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

size_t getNumElements(struct pumaList* list)
{
	size_t numThreads = pumaGetNumThreads(list->threadPool);
	size_t sums[numThreads];
	memset(sums, 0, numThreads * sizeof(size_t));

	struct _getNumElementsArg arg;
	arg.list = list;
	arg.sums = sums;

	executeOnThreadPool(list->threadPool, &_getNumElementsWorker, (void*)(&arg));

	size_t sum = 0;

	for(size_t i = 0; i < numThreads; ++i)
		sum += sums[i];

	return sum;
}

static void _destroyThreadListWorker(void* arg)
{
	struct pumaList* list = (struct pumaList*)arg;
	size_t thread = pumaGetThreadNum();
	struct pumaNode* currentNode = list->threadLists[thread].head;

	while(currentNode != NULL)
	{
		destroyPumaBitmask(currentNode->freeMask);

		struct pumaNode* nextNode = currentNode->next;
		free(currentNode);
		currentNode = nextNode;
	}
}

void destroyPumaList(struct pumaList* list)
{
	VALGRIND_MAKE_MEM_DEFINED(list->threadLists,
			list->numCores * sizeof(struct pumaThreadList));
	
	executeOnThreadPool(list->threadPool, &_destroyThreadListWorker, (void*)list);

	free(list->threadLists);
	free(list);
}
