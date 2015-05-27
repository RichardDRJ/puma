#include "pumakernel.h"

#include "internal/valgrind.h"
#include "internal/pumautil.h"
#include "internal/pumanode.h"
#include "internal/pumabalance.h"
#include "internal/pumathreadlist.h"
#include "internal/profiling.h"
#include "pumathreadpool.h"

#include <assert.h>
#include "internal/numa.h"

static void _runKernelOnNode(struct pumaNode* node,
		pumaKernel kernel, void* extraData)
{
	size_t skippedElements = 0;
	size_t originalElements = node->numElements;

	for(size_t i = 0; i - skippedElements < originalElements && i < node->capacity; ++i)
	{
		if(pumaBitmaskGet(&node->freeMask, i) == MASKFREE)
		{
			++skippedElements;
			continue;
		}

		void* currentElement = _getElement(node, i);
		kernel(currentElement, extraData);
	}
}

static void _runKernelThread(struct pumaSet* set, pumaKernel kernel,
		void* extraData)
{
	struct pumaThreadList* tl = _getListForCurrentThread(set);
	VALGRIND_MAKE_MEM_DEFINED(tl, sizeof(struct pumaThreadList));
	struct pumaNode* currentNode = tl->head;
	VALGRIND_MAKE_MEM_NOACCESS(tl, sizeof(struct pumaThreadList));

	assert(tl->numaDomain == _getCurrentNumaDomain());

	PROFILING_DECLS(runKernel);
	PROFILE(runKernel,
	while(currentNode != NULL && currentNode->active)
	{
		if(currentNode->dirty)
			_cleanupNode(currentNode, tl);

		_runKernelOnNode(currentNode, kernel, extraData);

		currentNode = currentNode->next;
	}
	)
	VALGRIND_MAKE_MEM_DEFINED(tl, sizeof(struct pumaThreadList));
	tl->totalRunTime -= tl->kernelRunTimes[tl->nextRunTimeSlot];
	tl->kernelRunTimes[tl->nextRunTimeSlot] = GET_ELAPSED_S(runKernel);
	tl->totalRunTime += tl->kernelRunTimes[tl->nextRunTimeSlot];
	++tl->nextRunTimeSlot;
	tl->nextRunTimeSlot %= NUM_KERNEL_RUNTIMES;
	VALGRIND_MAKE_MEM_NOACCESS(tl, sizeof(struct pumaThreadList));
}

void runKernelCurrentThread(struct pumaSet* set, pumaKernel kernel,
		struct pumaExtraKernelData* extraDataDetails)
{
	_balanceThreadLoad(set);
	void* extraData = extraDataDetails->extraDataConstructor(
			extraDataDetails->constructorData);
	
	_runKernelThread(set, kernel, extraData);

	extraDataDetails->extraDataThreadReduce(extraData);

	extraDataDetails->extraDataDestructor(extraData);
}

struct _runKernelArg
{
	struct pumaSet* set;
	pumaKernel* kernels;
	size_t numKernels;
	struct pumaExtraKernelData* extraDataDetails;
	void** extraData;
};

static void _runKernelWorker(void* voidArg)
{
	struct _runKernelArg* arg = (struct _runKernelArg*)voidArg;
	int thread = pumaGetThreadNum();
	arg->extraData[thread] = arg->extraDataDetails
			->extraDataConstructor(arg->extraDataDetails->constructorData);

	for(size_t i = 0; i < arg->numKernels; ++i)
		_runKernelThread(arg->set, arg->kernels[i], arg->extraData[thread]);

	arg->extraDataDetails->extraDataThreadReduce(arg->extraData[thread]);
}

static void _cleanupKernelWorker(void* voidArg)
{
	struct _runKernelArg* arg = (struct _runKernelArg*)voidArg;
	int thread = pumaGetThreadNum();
	arg->extraDataDetails->extraDataDestructor(arg->extraData[thread]);
}

void runKernelList(struct pumaSet* set, pumaKernel kernels[],
		size_t numKernels, struct pumaExtraKernelData* extraDataDetails)
{
	_balanceThreadLoad(set);

	unsigned int numThreads = pumaGetNumThreads(set->threadPool);
	void* extraData[numThreads];

	if(extraDataDetails == NULL)
		extraDataDetails = &emptyKernelData;

	struct _runKernelArg arg;
	arg.set = set;
	arg.kernels = kernels;
	arg.numKernels = numKernels;
	arg.extraDataDetails = extraDataDetails;
	arg.extraData = extraData;

	executeOnThreadPool(set->threadPool, _runKernelWorker, (void*)(&arg));

	extraDataDetails->extraDataReduce(extraDataDetails->retValue,
			extraData, numThreads);

	executeOnThreadPool(set->threadPool, _cleanupKernelWorker, (void*)(&arg));
}

void runKernel(struct pumaSet* set, pumaKernel kernel,
		struct pumaExtraKernelData* extraDataDetails)
{
	runKernelList(set, &kernel, 1, extraDataDetails);
}
