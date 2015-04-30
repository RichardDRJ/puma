#ifndef _PUMALIST__PUMA_THREAD_LIST_H_
#define _PUMALIST__PUMA_THREAD_LIST_H_

#include <stdlib.h>
#include "internal/pumanode.h"

#define NUM_KERNEL_RUNTIMES 5

struct pumaList;

struct pumaThreadList
{
	struct pumaNode* head;
	struct pumaNode* tail;
	size_t numElements;
	size_t numNodes;
	bool active;
	int numaDomain;

	size_t elementSize;

	size_t toNodes;
	size_t fromNodes;

	int tid;

	double relativeSpeed;
	double totalRunTime;
	size_t nextRunTimeSlot;
	double kernelRunTimes[NUM_KERNEL_RUNTIMES];
};

struct pumaThreadList* _getListForCurrentThread(struct pumaList* list);
void _emptyThreadList(struct pumaThreadList* tl);

#endif // _PUMALIST__PUMA_THREAD_LIST_H_
