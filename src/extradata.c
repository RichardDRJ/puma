#include "extradata.h"

#include <stdlib.h>

static void* _emptyExtraDataConstructor(void* constructorData)
{
	return NULL;
}

static void _emptyExtraDataDestructor(void* data)
{
	
}

static void _emptyExtraDataReduce(void* retValue, void* data[],
		unsigned int nThreads)
{

}

static void _emptyExtraDataPerThreadReduce(void* data)
{

}

struct pumaListExtraKernelData emptyKernelData = {
		&_emptyExtraDataConstructor,
		NULL,
		&_emptyExtraDataDestructor,
		&_emptyExtraDataPerThreadReduce,
		&_emptyExtraDataReduce,
		NULL,
};