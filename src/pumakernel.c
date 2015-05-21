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

static void _runKernelThread(struct pumaList* list, pumaKernel kernel,
		struct pumaListExtraKernelData* extraDataDetails, void* extraData)
{
	struct pumaThreadList* tl = _getListForCurrentThread(list);
	VALGRIND_MAKE_MEM_DEFINED(tl, sizeof(struct pumaThreadList));
	struct pumaNode* currentNode = tl->head;
	VALGRIND_MAKE_MEM_NOACCESS(tl, sizeof(struct pumaThreadList));

	PROFILING_DECLS(runKernel);
	PROFILE(runKernel,
	while(currentNode != NULL && currentNode->active)
	{
		if(currentNode->dirty)
			_cleanupNode(currentNode, tl);

#if !defined(NDEBUG) && !defined(NNUMA)
		size_t numPages = currentNode->numPages;
		int nodes[numPages];

		size_t pumaPageSize = (size_t)sysconf(_SC_PAGESIZE);

		for(size_t p = 0; p < numPages; ++p)
			get_mempolicy(&nodes[p], NULL, 0, (void*)currentNode + p * pumaPageSize, MPOL_F_NODE | MPOL_F_ADDR);

		for(size_t p = 0; p < numPages; ++p)
			assert(nodes[p] == tl->numaDomain);
#endif

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

void runKernelCurrentThread(struct pumaList* list, pumaKernel kernel,
		struct pumaListExtraKernelData* extraDataDetails)
{
	_balanceThreadLoad(list);
	void* extraData = extraDataDetails->extraDataConstructor(
			extraDataDetails->constructorData);
	
	_runKernelThread(list, kernel, extraDataDetails, extraData);

	extraDataDetails->extraDataThreadReduce(extraData);

	extraDataDetails->extraDataDestructor(extraData);
}

struct _runKernelArg
{
	struct pumaList* list;
	pumaKernel* kernels;
	size_t numKernels;
	struct pumaListExtraKernelData* extraDataDetails;
	void** extraData;
};

static void _runKernelWorker(void* voidArg)
{
	struct _runKernelArg* arg = (struct _runKernelArg*)voidArg;
	int thread = pumaGetThreadNum();
	arg->extraData[thread] = arg->extraDataDetails
			->extraDataConstructor(arg->extraDataDetails->constructorData);

	for(size_t i = 0; i < arg->numKernels; ++i)
		_runKernelThread(arg->list, arg->kernels[i], arg->extraDataDetails,
				arg->extraData[thread]);

	arg->extraDataDetails->extraDataThreadReduce(arg->extraData[thread]);
}

static void _cleanupKernelWorker(void* voidArg)
{
	struct _runKernelArg* arg = (struct _runKernelArg*)voidArg;
	int thread = pumaGetThreadNum();
	arg->extraDataDetails->extraDataDestructor(arg->extraData[thread]);
}

void runKernelList(struct pumaList* list, pumaKernel kernels[],
		size_t numKernels, struct pumaListExtraKernelData* extraDataDetails)
{
	_balanceThreadLoad(list);

	unsigned int numThreads = pumaGetNumThreads(list->threadPool);
	void* extraData[numThreads];

	if(extraDataDetails == NULL)
		extraDataDetails = &emptyKernelData;

	struct _runKernelArg arg;
	arg.list = list;
	arg.kernels = kernels;
	arg.numKernels = numKernels;
	arg.extraDataDetails = extraDataDetails;
	arg.extraData = extraData;

	executeOnThreadPool(list->threadPool, _runKernelWorker, (void*)(&arg));

	extraDataDetails->extraDataReduce(extraDataDetails->retValue,
			extraData, numThreads);

	executeOnThreadPool(list->threadPool, _cleanupKernelWorker, (void*)(&arg));
}

void runKernel(struct pumaList* list, pumaKernel kernel,
		struct pumaListExtraKernelData* extraDataDetails)
{
	runKernelList(list, &kernel, 1, extraDataDetails);
}
