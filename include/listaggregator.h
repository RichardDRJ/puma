#ifndef __PUMALIST__LISTAGGREGATOR_H__
#define __PUMALIST__LISTAGGREGATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pumalist.h"
#include "extradata.h"
#include "linkedlist.h"

void puma_initLinkedListExtraKernelData(struct pumaListExtraKernelData* data,
		struct puma_linkedList* list);

#ifdef __cplusplus
}
#endif

#endif // __PUMALIST__LIST_AGGREGATOR_H__