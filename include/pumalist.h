/**
	@file pumalist.h
	@author Richard Jones (joneseh25@gmail.com)
	@date May 2015
	@brief Declarations of the main pumalist functions and struct.
*/

#ifndef __PUMALIST__PUMALIST_H__
#define __PUMALIST__PUMALIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

struct pumaThreadPool;
struct pumaThreadList;
struct pumaDomain;
struct pumaList;

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

struct pumaList
{
	// Public members:
	/** @brief The thread pool which is used for running kernels on this list. */
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
	@brief Allocate a new pumaList.

	@param elementSize Size of each element in the list.
	@param numThreads The number of threads we want to run pumaList on.
	@param threadAffinity An affinity string specifying the CPUs we want to bind
		to. Can contain numbers separated either by commas or dashes.
		"i-j" means bind to every cpu from i to j inclusive. "i,j" means bind to
		i and j. Formats can be mixed: "0-3, 6, 10, 12, 15, 13" is valid.
		If NULL, we simply bind each thread to the CPU whose number matches the
		thread (tid 0 == cpu 0 :: tid 1 == cpu 1 :: etc.).
*/
struct pumaList* createPumaList(size_t elementSize, size_t numThreads,
		char* threadAffinity);

/** @brief Free list and destroy all of its members. */
void destroyPumaList(struct pumaList* list);

/** @return the total number of elements in the list. */
size_t getNumElements(struct pumaList* list);

/** @brief Set the balancing strategy.

	Set splitter as the balancing function to use when pumallocManualBalancing()
	is called. If autoBalance is false, don't automatically balance thread load
	each time a kernel is run.
	If it's true, we balance based on previous execution time and the amount of
	data associated with each thread.
*/
void pumaListSetBalancer(struct pumaList* list, bool autoBalance,
		splitterFunc* splitter, void* splitterExtraData);

#ifdef __cplusplus
}
#endif

#endif // __PUMALIST__PUMALIST_H__
