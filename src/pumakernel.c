#include "pumakernel.h"

#include "internal/valgrind.h"
#include "internal/pumautil.h"
#include "internal/pumanode.h"
#include "internal/pumabalance.h"
#include "internal/pumathreadlist.h"
#include "internal/pumathreadpool.h"

#include <omp.h>
#include <assert.h>

static void _runKernelOnNode(struct pumaNode* node,
		pumaKernel kernel, void* extraData)
{
	size_t skippedElements = 0;
	size_t originalElements = node->numElements;

	for(size_t i = 0; i - skippedElements < originalElements; ++i)
	{
		if(pumaBitmaskGet(node->freeMask, i) == MASKFREE)
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
	struct pumaThreadList* threadList = _getListForCurrentThread(list);
	VALGRIND_MAKE_MEM_DEFINED(threadList, sizeof(struct pumaThreadList));
	struct pumaNode* currentNode = threadList->head;
	VALGRIND_MAKE_MEM_NOACCESS(threadList, sizeof(struct pumaThreadList));

	while(currentNode != NULL && currentNode->active)
	{
		if(currentNode->dirty)
			_cleanupNode(currentNode, threadList);

		_runKernelOnNode(currentNode, kernel, extraData);

		currentNode = currentNode->next;
	}
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
	int thread = _pumaGetThreadPoolNumber();
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
	int thread = _pumaGetThreadPoolNumber();
	arg->extraDataDetails->extraDataDestructor(arg->extraData[thread]);
}

void runKernelList(struct pumaList* list, pumaKernel kernels[],
		size_t numKernels, struct pumaListExtraKernelData* extraDataDetails)
{
	_balanceThreadLoad(list);

	unsigned int numThreads = omp_get_max_threads();
	void* extraData[numThreads];

	if(extraDataDetails == NULL)
		extraDataDetails = &emptyKernelData;

	struct _runKernelArg arg;
	arg.list = list;
	arg.kernels = kernels;
	arg.numKernels = numKernels;
	arg.extraDataDetails = extraDataDetails;
	arg.extraData = extraData;

	_executeOnThreadPool(list->threadPool, _runKernelWorker, (void*)extraData);

	extraDataDetails->extraDataReduce(extraDataDetails->retValue,
			extraData, numThreads);

	_executeOnThreadPool(list->threadPool, _cleanupKernelWorker, (void*)extraData);
}

void runKernel(struct pumaList* list, pumaKernel kernel,
		struct pumaListExtraKernelData* extraDataDetails)
{
	runKernelList(list, &kernel, 1, extraDataDetails);
}