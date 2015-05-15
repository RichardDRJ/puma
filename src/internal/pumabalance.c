#include "internal/pumabalance.h"
#include "internal/pumathreadlist.h"
#include "internal/pumadomain.h"
#include "internal/pumanode.h"
#include "internal/numa.h"
#include "internal/valgrind.h"

#include "pumalist.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

struct _pumaDomainBal
{
	struct pumaDomain dom;
	long offset;
};

static void _moveNodeDomains(struct pumaNode* start, struct pumaNode* end, int domain)
{
#ifndef NNUMA
	bool done = false;

	while(!done)
	{
		done = (start == end);

		size_t numPages = end->numPages;
		int nodes[numPages];

		for(size_t p = 0; p < numPages; ++p)
			nodes[p] = domain;

		void* pages[numPages];
		pages[0] = start;
		size_t pumaPageSize = (size_t)sysconf(_SC_PAGESIZE);

		for(size_t p = 1; p < numPages; ++p)
			pages[p] = pages[0] + p * pumaPageSize;

		numa_move_pages(0, numPages, pages, nodes, NULL, 0);

		start = start->next;
	}
#else
	(void)start; (void)end; (void)domain;
#endif
}

static void _indexNodes(struct pumaNode* start, struct pumaNode* end)
{
	bool done = false;
	size_t index = (start->prev == NULL) ? 0 : start->prev->index + 1;

	while(!done)
	{
		done = (start == end);
		start->index = index++;
		start = start->next;
	}
}

static void _moveNodeThreadLists(struct pumaNode* start, struct pumaNode* end,
		struct pumaThreadList* tl)
{
	bool done = false;

	while(!done)
	{
		done = (start == end);
		start->threadList = tl;
		start = start->next;
	}
}

static void _updatenumElements(struct pumaNode* start, struct pumaNode* end,
		struct pumaThreadList* from, struct pumaThreadList* to)
{
	bool done = false;

	while(!done)
	{
		done = (start == end);
		from->numElements -= start->numElements;
		to->numElements += start->numElements;
		start = start->next;
	}
}

size_t _countNodes(struct pumaThreadList* tl)
{
	size_t n = 0;
	struct pumaNode* c = tl->head;
	while(c != NULL && c->active)
	{
		assert(c->index == n);
		++n;
		c = c->next;
	}

	return n;
}

void _transferNNodes(size_t n, struct pumaThreadList* from,
		struct pumaThreadList* to)
{
	assert(n > 0);
	assert(from->numNodes >= n);
	assert(to->numNodes == _countNodes(to) || (printf("%lu != %lu\n", to->numNodes, _countNodes(to)), false));
	assert(from->numNodes == _countNodes(from) || (printf("%lu != %lu\n", from->numNodes, _countNodes(from)), false));

	struct pumaNode* start = from->tail;
	struct pumaNode* end = from->tail;
	struct pumaNode* fromTailNext = from->tail->next;

	for(size_t i = 1; i < n; ++i)
		start = start->prev;

	from->tail = start->prev;

	if(to->tail != NULL)
	{
		end->next = to->tail->next;
		if(end->next != NULL)
			end->next->prev = end;
		to->tail->next = start;
		to->tail->dirty = true;
		start->prev = to->tail;
	}
	else
	{
		to->head = start;
		end->next = NULL;
		start->prev = NULL;
	}

	to->tail = end;

	if(from->tail == NULL)
		from->head = from->tail = fromTailNext;
	else
	{
		from->tail->next = fromTailNext;
		if(fromTailNext != NULL)
			fromTailNext->prev = from->tail;
	}

	if(to->numaDomain != from->numaDomain)
		_moveNodeDomains(start, end, to->numaDomain);

	_indexNodes(start, end);
	_moveNodeThreadLists(start, end, to);
	_updatenumElements(start, end, from, to);

	from->numNodes -= n;
	to->numNodes += n;

	assert(to->numNodes == _countNodes(to));
	assert(from->numNodes == _countNodes(from));
}

size_t _min(size_t a, size_t b)
{
	return (a < b) * a + (a >= b) * b;
}

static void _autobalanceThreadLoad(struct pumaList* list)
{
	long avgNodes = 0;
	size_t numLists = 0;
	double totalRunTime = 0;

	for(size_t i = 0; i < list->numCores; ++i)
	{
		struct pumaThreadList* tl = list->threadLists + i;
		VALGRIND_MAKE_MEM_DEFINED(tl, sizeof(struct pumaThreadList));

		totalRunTime += tl->totalRunTime;
		avgNodes += tl->numNodes * (tl->active == true);
		numLists += (tl->active == true);
		VALGRIND_MAKE_MEM_NOACCESS(tl, sizeof(struct pumaThreadList));
	}

	for(size_t i = 0; i < list->numCores; ++i)
	{
		struct pumaThreadList* tl = list->threadLists + i;
		VALGRIND_MAKE_MEM_DEFINED(tl, sizeof(struct pumaThreadList));
		tl->relativeSpeed = ((numLists + 1.0) / numLists) - (tl->totalRunTime / totalRunTime);
		VALGRIND_MAKE_MEM_NOACCESS(tl, sizeof(struct pumaThreadList));
	}

	struct pumaThreadList* negStack[numLists];
	size_t negStackSize = 0;

	struct pumaThreadList* posStack[numLists];
	size_t posStackSize = 0;

	size_t rem = avgNodes % numLists;
	avgNodes /= numLists;

	for(size_t d = 0; d < list->numDomains; ++d)
	{
		struct pumaDomain* currDomain = list->domains + d;
		for(size_t c = 0; c < currDomain->numListsInDomain; ++c)
		{
			struct pumaThreadList* tl = currDomain->listsInDomain + c;
			VALGRIND_MAKE_MEM_DEFINED(tl, sizeof(struct pumaThreadList));
			if(!tl->active)
				continue;

			size_t tlAvgNodes = (tl->relativeSpeed) * avgNodes;

			rem -= ((rem > 0) && (tl->numNodes == tlAvgNodes + 1));

			if(tl->numNodes > tlAvgNodes + (rem > 0))
				posStack[posStackSize++] = tl;
			else if(tl->numNodes < tlAvgNodes)
				negStack[negStackSize++] = tl;
			VALGRIND_MAKE_MEM_NOACCESS(tl, sizeof(struct pumaThreadList));
		}

		while(posStackSize > 0 && negStackSize > 0)
		{
			VALGRIND_MAKE_MEM_DEFINED(negStack[negStackSize - 1], sizeof(struct pumaThreadList));
			VALGRIND_MAKE_MEM_DEFINED(posStack[posStackSize - 1], sizeof(struct pumaThreadList));
			size_t posAvgNodes = (posStack[posStackSize - 1]->relativeSpeed) * avgNodes;
			size_t negAvgNodes = (negStack[negStackSize - 1]->relativeSpeed) * avgNodes;
			size_t nToTransfer =
					_min(negAvgNodes - negStack[negStackSize - 1]->numNodes,
					posStack[posStackSize - 1]->numNodes - (posAvgNodes + (rem > 0)));

			_transferNNodes(nToTransfer, posStack[posStackSize - 1],
					negStack[negStackSize - 1]);

			bool posIsTarget = (posStack[posStackSize - 1]->numNodes == posAvgNodes + (rem > 0));
			rem -= ((rem > 0) && posIsTarget);
			posStackSize -= (posIsTarget);

			bool negIsTarget = (negStack[negStackSize - 1]->numNodes == negAvgNodes);
			negStackSize -= (negIsTarget);
			VALGRIND_MAKE_MEM_NOACCESS(negStack[negStackSize - 1], sizeof(struct pumaThreadList));
			VALGRIND_MAKE_MEM_NOACCESS(posStack[posStackSize - 1], sizeof(struct pumaThreadList));
		}
	}
}

void _balanceThreadLoad(struct pumaList* list)
{
	if(list->autoBalance)
		_autobalanceThreadLoad(list);
}
