#include "listaggregator.h"

static void* _puma_listExtraDataConstructor(void* constructorData)
{
	return puma_createLinkedList();
}

static void _puma_listExtraDataDestructor(void* data)
{
	puma_deleteLinkedList((struct puma_linkedList*)data, NULL);
}

static void _puma_listExtraDataReduce(void* retValue, void* data[],
		unsigned int nThreads)
{
	struct puma_linkedList* retList = (struct puma_linkedList*)retValue;
	for(int i = 0; i < nThreads; ++i)
	{
		struct puma_linkedList* currentList = (struct puma_linkedList*)data[i];
		void* element;

		while(element = puma_popHeadFromLinkedList(currentList))
			puma_insertIntoLinkedList(retList, element, NULL);
	}
}
void puma_initLinkedListExtraKernelData(struct pumaListExtraKernelData* data,
		struct puma_linkedList* list)
{
	*data = emptyKernelData;
	data->extraDataConstructor = &_puma_listExtraDataConstructor;
	data->extraDataDestructor = &_puma_listExtraDataDestructor;
	data->extraDataReduce = &_puma_listExtraDataReduce;
	data->retValue = list;
}