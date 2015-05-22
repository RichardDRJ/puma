/**
	@file extradata.h
	@author Richard Jones (joneseh25@gmail.com)
	@date May 2015
	@brief Definitions of stuff relating to extra data for kernels.
*/

#ifndef _PUMALIST__EXTRADATA_H_
#define _PUMALIST__EXTRADATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
	@brief
		Extra data descriptor to pass into runKernel().
*/
struct pumaListExtraKernelData
{
	/** A per-thread constructor for extra data to pass into your kernel. */
	void* (*extraDataConstructor)(void* constructorData);

	/** A pointer to anything you might wish to pass into the constructor.
		May be NULL. */
	void* constructorData;

	/** A destructor to match extraDataConstructor(). */
	void (*extraDataDestructor)(void* data);

	/** A per-thread reduction/finalisation function which is run after the
		kernel is run on each thread.
		@param data The per-thread extradata which was passed into each kernel
			on this thread.
	*/
	void (*extraDataThreadReduce)(void* data);

	/** A global reduction/finalisation function which is run after all threads
		have finished running the kernel.
		@param retValue User-specified pointer to a return value.
		@param data Array of the extradata for all threads.
		@param nThreads Number of threads in use for the kernel run.
	*/
	void (*extraDataReduce)(void* retValue, void* data[],
			unsigned int nThreads);

	/** A pointer to a return value to be used by extraDataReduce.
		May be NULL. */
	void* retValue;
};

/**
	@brief
		Initialise kernelData with each of the subsequent arguments.
		Any or all of the arguments after kernelData may be NULL, in which case
		they are set to point to dummy functions which do nothing.
*/
void initKernelData(struct pumaListExtraKernelData* kernelData,
		void* (*extraDataConstructor)(void* constructorData),
		void* constructorData,
		void (*extraDataDestructor)(void* data),
		void (*extraDataThreadReduce)(void* data),
		void (*extraDataReduce)(void* retValue, void* data[],
				unsigned int nThreads),
		void* retValue);

/**
	@brief
		A dummy extradata structure which does nothing and causes NULL to be
		passed as extra data to kernels.
*/
extern struct pumaListExtraKernelData emptyKernelData;

#ifdef __cplusplus
}
#endif

#endif // _PUMALIST__EXTRADATA_H_
