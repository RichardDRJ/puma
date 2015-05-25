/**
	@file pumaset.h
	@author Richard Jones (joneseh25@gmail.com)
	@date May 2015
	@brief Declarations of the main pumalist functions and struct.
*/

#ifndef __PUMA__PUMASET_H__
#define __PUMA__PUMASET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

struct pumaThreadPool;
struct pumaThreadList;
struct pumaDomain;

/**
	@brief Signature for a function which, given an element, the total number of
		threads and some optional extra data, will specify the thread to place
		the element on.

	Set with pumaListSetBalancer() and only used if autoBalance is true.

	@param element The element to place.
	@param numThreads The total number of threads in use.
	@param extraData Optional extra data set with pumaListSetBalancer().
*/
typedef size_t (splitterFunc)(void* element, size_t numThreads, void* extraData);

struct pumaSet
{
	// Public members:
	/** @brief The thread pool which is used for running kernels on this set. */
	struct pumaThreadPool* threadPool;
	size_t elementSize;

	// Private members:
	struct pumaDomain* domains;
	size_t numDomains;
	struct pumaThreadList* threadLists;
	size_t numCores;
	size_t numThreads;

	size_t* threadListToIndex;

	bool autoBalance;
	splitterFunc* splitter;
	void* splitterExtraData;
};

/**
	@brief Allocate a new pumaSet.

	@param elementSize Size of each element in the set.
	@param numThreads The number of threads we want to run pumaSet on.
	@param threadAffinity An affinity string specifying the CPUs we want to bind
		to. Can contain numbers separated either by commas or dashes.
		"i-j" means bind to every cpu from i to j inclusive. "i,j" means bind to
		i and j. Formats can be mixed: "0-3, 6, 10, 12, 15, 13" is valid.
		If NULL, we simply bind each thread to the CPU whose number matches the
		thread (tid 0 == cpu 0 :: tid 1 == cpu 1 :: etc.).
*/
struct pumaSet* createPumaSet(size_t elementSize, size_t numThreads,
		char* threadAffinity);

/** @brief Free set and destroy all of its members. */
void destroyPumaSet(struct pumaSet* set);

/** @return the total number of elements in the set. */
size_t getNumElements(struct pumaSet* set);

/** @brief Set the balancing strategy.

	Set splitter as the balancing function to use when pumallocManualBalancing()
	is called. If autoBalance is false, don't automatically balance thread load
	each time a kernel is run.
	If it's true, we balance based on previous execution time and the amount of
	data associated with each thread.
*/
void pumaListSetBalancer(struct pumaSet* set, bool autoBalance,
		splitterFunc* splitter, void* splitterExtraData);

#ifdef __cplusplus
}
#endif

#endif // __PUMA__PUMASET_H__
