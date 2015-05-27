#include "extradata.h"

#include <stdlib.h>

static void* _emptyExtraDataConstructor(void* constructorData)
{
	(void)constructorData;
	return NULL;
}

static void _emptyExtraDataDestructor(void* data)
{
	(void)data;
}

static void _emptyExtraDataReduce(void* retValue, void* data[],
		unsigned int nThreads)
{
	(void)retValue;
	(void)data;
	(void)nThreads;
}

static void _emptyExtraDataPerThreadReduce(void* data)
{
	(void)data;
}

struct pumaExtraKernelData emptyKernelData = {
		&_emptyExtraDataConstructor,
		NULL,
		&_emptyExtraDataDestructor,
		&_emptyExtraDataPerThreadReduce,
		&_emptyExtraDataReduce,
		NULL,
};

void initKernelData(struct pumaExtraKernelData* kernelData,
		void* (*extraDataConstructor)(void* constructorData),
		void* constructorData,
		void (*extraDataDestructor)(void* data),
		void (*extraDataThreadReduce)(void* data),
		void (*extraDataReduce)(void* retValue, void* data[],
				unsigned int nThreads),
		void* retValue)
{
	*kernelData = emptyKernelData;

	if(extraDataConstructor != NULL)
		kernelData->extraDataConstructor = extraDataConstructor;
	if(constructorData != NULL)
		kernelData->constructorData = constructorData;
	if(extraDataDestructor != NULL)
		kernelData->extraDataDestructor = extraDataDestructor;
	if(extraDataThreadReduce != NULL)
		kernelData->extraDataThreadReduce = extraDataThreadReduce;
	if(extraDataReduce != NULL)
		kernelData->extraDataReduce = extraDataReduce;
	if(retValue != NULL)
		kernelData->retValue = retValue;
}
