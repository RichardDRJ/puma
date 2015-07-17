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
	set->tidToThreadList[tl->tid] = tl;
	tl->elementSize = set->elementSize;
}

void pumaSetBalancer(struct pumaSet* set, bool autoBalance,
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
	newList->numCores = numCores;
	newList->numThreads = numThreads;
#ifdef STATIC_THREADPOOL
	setupThreadPool(numThreads, threadAffinity);
	newList->threadPool = getThreadPool();
#else
	newList->threadPool = createThreadPool(numThreads, threadAffinity);
#endif // STATIC_THREADPOOL
	newList->tidToThreadList = calloc(numThreads, sizeof(size_t));

	size_t numDomains = _getNumDomains();
	newList->numDomains = numDomains;
	newList->domains =
			(struct pumaDomain*)calloc(numDomains, sizeof(struct pumaDomain));
	newList->autoBalance = true;

	for(size_t i = 0; i < numDomains; ++i)
		initDomain(&newList->domains[i], i);

	executeOnThreadPool(newList->threadPool, &_setupThreadListsWorker, (void*)newList);

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

	struct pumaThreadList* threadList = _getListForCurrentThread(set);
	size_t thread = pumaGetThreadNum();

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
	struct pumaThreadList* threadList = _getListForCurrentThread(set);
	struct pumaNode* currentNode = threadList->head;

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
	executeOnThreadPool(set->threadPool, &_destroyThreadListWorker, (void*)set);

	free(set->tidToThreadList);

	for(size_t i = 0; i < set->numDomains; ++i)
		destroyDomain(&set->domains[i]);

	free(set->domains);

	free(set);
}
