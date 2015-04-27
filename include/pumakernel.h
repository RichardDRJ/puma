#ifndef _PUMALIST__PUMA_KERNEL_H_
#define _PUMALIST__PUMA_KERNEL_H_

#include "pumalist.h"
#include "extradata.h"

typedef void (*pumaKernel)(void* element, void* extraData);

void runKernel(struct pumaList* list, pumaKernel kernel,
		struct pumaListExtraKernelData* extraDataDetails);
void runKernelList(struct pumaList* list, pumaKernel kernels[],
		size_t numKernels, struct pumaListExtraKernelData* extraDataDetails);
void runKernelCurrentThread(struct pumaList* list, pumaKernel kernel,
		struct pumaListExtraKernelData* extraDataDetails);

#endif // _PUMALIST__PUMA_KERNEL_H_
