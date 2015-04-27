#include "internal/numa.h"
#include "pumalist.h"
#include "internal/valgrind.h"
#include "internal/pumalog.h"
#include "internal/pumanode.h"
#include "internal/pumaheader.h"
#include "internal/pumathreadlist.h"
#include "internal/pumadomain.h"
#include "internal/pumautil.h"

#include <stdlib.h>
#include <sched.h>
#include <omp.h>

#include <string.h>

#include <errno.h>

#include <assert.h>

struct pumaList* createPumaList(size_t elementSize)
{
	struct pumaList* newList =
			(struct pumaList*)malloc(sizeof(struct pumaList));

	if(pumaPageSize == 0)
		pumaPageSize = (size_t)sysconf(_SC_PAGESIZE);

	size_t numCores = _getNumCPUs();

	newList->elementSize = elementSize;
	newList->threadLists = (struct pumaThreadList*)calloc(numCores, sizeof(struct pumaThreadList));
	newList->numCores = numCores;

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

	#pragma omp parallel
	{
		int currDomain = _getCurrentNumaDomain();
		struct pumaDomain* domain = &newList->domains[currDomain];
		size_t cpuIndex = _getCurrentCPUIndexInDomain();
		struct pumaThreadList* tl = &domain->listsInDomain[cpuIndex];
		tl->active = true;
		tl->numaDomain = currDomain;
		tl->tid = omp_get_thread_num();
	}

	for(size_t i = 0; i < numCores; ++i)
		VALGRIND_MAKE_MEM_NOACCESS(newList->threadLists + i, sizeof(struct pumaThreadList));

	return newList;
}

size_t getNumElements(struct pumaList* list)
{
	size_t sum = 0;

	#pragma omp parallel reduction(+:sum)
	{
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
		sum += threadList->numElements;
		VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
	}

	return sum;
}

size_t getNumElementsMatcher(struct pumaList* list,
		bool (*matcher)(void* element, void* extraData), void* extraData)
{
	unsigned int numThreads = omp_get_max_threads();
	size_t numElements[numThreads];

	for(int i = 0; i < numThreads; ++i)
		numElements[i] = 0;

	#pragma omp parallel
	{
		unsigned int thread = omp_get_thread_num();

		struct pumaThreadList* threadList = &list->threadLists[thread];

		VALGRIND_MAKE_MEM_DEFINED(threadList, sizeof(struct pumaThreadList));

		struct pumaNode* currentNode = threadList->head;

		while(currentNode != NULL && currentNode->active)
		{
			for(int i = 0; i < currentNode->capacity; ++i)
			{
				if(pumaBitmaskGet(currentNode->freeMask, i) == MASKFREE)
					continue;

				void* element = _getElement(currentNode, i);
				if(matcher(element, extraData))
					++numElements[thread];
			}

			currentNode = currentNode->next;
		}

		VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
	}

	size_t sum = 0;

	for(int i = 0; i < numThreads; ++i)
		sum += numElements[i];

	return sum;
}

void getPerThreadNumElements(struct pumaList* list, size_t numElements[])
{
	#pragma omp parallel
	{
		int currDomain = _getCurrentNumaDomain();
		struct pumaDomain* domain = &list->domains[currDomain];
		struct pumaThreadList* threadList =
				&domain->listsInDomain[_getCurrentCPUIndexInDomain()];

		int thread = omp_get_thread_num();

		VALGRIND_MAKE_MEM_DEFINED(threadList, sizeof(struct pumaThreadList));
		numElements[thread] = threadList->numElements;
		VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
	}
}

void getPerThreadNumNodes(struct pumaList* list, size_t* numNodes)
{
	#pragma omp parallel
	{
		int currDomain = _getCurrentNumaDomain();
		struct pumaDomain* domain = &list->domains[currDomain];
		struct pumaThreadList* threadList =
				&domain->listsInDomain[_getCurrentCPUIndexInDomain()];

		int thread = omp_get_thread_num();

		VALGRIND_MAKE_MEM_DEFINED(threadList, sizeof(struct pumaThreadList));
		numNodes[thread] = threadList->numNodes;
		VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));
	}
}

void destroyPumaList(struct pumaList* list)
{
	VALGRIND_MAKE_MEM_DEFINED(list->threadLists,
			list->numCores * sizeof(struct pumaThreadList));
	#pragma omp parallel for
	for(size_t i = 0; i < list->numCores; ++i)
	{
		struct pumaNode* currentNode = list->threadLists[i].head;
		while(currentNode != NULL)
		{
			destroyPumaBitmask(currentNode->freeMask);

			struct pumaNode* nextNode = currentNode->next;
			free(currentNode);
			currentNode = nextNode;
		}
	}

	free(list->threadLists);
	free(list);
}
