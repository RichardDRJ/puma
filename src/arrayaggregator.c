#include "pumalist.h"
#include "arrayaggregator.h"
#include <assert.h>

static void* _puma_arrayExtraDataConstructor(void* constructorData)
{
	return calloc(1, (size_t)constructorData);
}

static void _puma_arrayExtraDataDestructor(void* data)
{
	free(data);
}

static void _puma_arrayExtraDataEmptyReduce(void* retValue, void* data[],
		unsigned int nThreads)
{

}

void puma_initArrayExtraKernelData(struct pumaListExtraKernelData* data,
		size_t arraySize)
{
	*data = emptyKernelData;
	assert(sizeof(size_t) <= sizeof(void*));
	data->extraDataConstructor = &_puma_arrayExtraDataConstructor;
	data->extraDataDestructor = &_puma_arrayExtraDataDestructor;
	data->extraDataReduce = &_puma_arrayExtraDataEmptyReduce;
	data->constructorData = (void*)arraySize;
}