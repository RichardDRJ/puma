/**
	@file pumakernel.h
	@author Richard Jones (joneseh25@gmail.com)
	@date May 2015
	@brief Declarations of kernel running functions.
*/

#ifndef _PUMA__PUMA_KERNEL_H_
#define _PUMA__PUMA_KERNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pumaset.h"
#include "extradata.h"

/**
	The type signature for kernels running on pumaList.
	\param element The current element in our runKernel iteration.
	\param extraData Extra data created using struct pumaExtraKernelData.
*/
typedef void (*pumaKernel)(void* element, void* extraData);

/**
	\brief Run kernel on all elements in set in parallel.

	\param set The pumaList containing our data.
	\param kernel The kernel we want to run.
	\param extraDataDetails Struct specifying what extra data we want our kernel
		to receive.
*/
void runKernel(struct pumaSet* set, pumaKernel kernel,
		struct pumaExtraKernelData* extraDataDetails);

/**
	\brief Run an array of kernels on all elements in set in parallel.

	\param set The pumaList containing our data.
	\param kernels The kernels we want to run.
	\param numKernels How many kernels are contained in kernels
	\param extraDataDetails Struct specifying what extra data we want our
		kernels to receive.
*/
void runKernelList(struct pumaSet* set, pumaKernel kernels[],
		size_t numKernels, struct pumaExtraKernelData* extraDataDetails);

/**
	\brief Run a kernel on all elements in set which are associated with the
		current thread.

	\param set The pumaList containing our data.
	\param kernel The kernel we want to run.
	\param extraDataDetails Struct specifying what extra data we want our kernel
		to receive.
*/
void runKernelCurrentThread(struct pumaSet* set, pumaKernel kernel,
		struct pumaExtraKernelData* extraDataDetails);

#ifdef __cplusplus
}
#endif

#endif // _PUMA__PUMA_KERNEL_H_
