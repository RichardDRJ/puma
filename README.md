PUMA
====

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [Compilation](#compilation)
  - [Dependencies](#dependencies)
  - [Configuration](#configuration)
- [Future Work](#future-work)
- [API Reference](#api-reference)
  - [PUMA Set Management](#puma-set-management)
  - [Memory Allocation](#memory-allocation)
  - [Kernel Application](#kernel-application-1)
  - [Static Data Allocation](#static-data-allocation)
- [Getting Started: Standard Deviation Hello World!](#getting-started-standard-deviation-hello-world)
- [Licence](#licence)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

PUMA is a smart memory allocator that manages data in a NUMA-aware way. It
exposes an interface to execute a kernel on the data in parallel, automatically
ensuring that each core which runs the kernel accesses primarily local memory.
It also provides an optional (primitive) time-based load balancer which can
adapt workloads to cases where some cores may have be less powerful or have more
to do per kernel invocation than others.

PUMA consists of several parts:

-   A NUMA-aware dynamic memory allocator for homogeneous elements;

-   An allocator for thread-local static data which cannot be freed
    individually. This allocator uses pools which are located on the
    domain to which the core associated with the thread belongs;

-   A parallel iteration interface which applies a kernel to all
    elements in a PUMA set;

-   A balancer which changes which threads blocks of data are associated
    with in order to balance kernel runtime across cores.

Compilation
===========

Compilation of PUMA requires a simple `make` invocation in the PUMA root
directory. The make targets are as follows:

-   all: Build PUMA and docs and run unit tests

-   doc: Build documentation with doxygen

-   no\_test: Build PUMA without running unit tests

-   clean: Clear the working tree

-   docs\_clean: Clear all built documentation

Dependencies
------------

PUMA relies on the following:

-   libNUMA

-   C99 compatible compiler

-   Valgrind (optional)

-   OpenMP (optional)

-   Doxygen (optional, documentation)

Configuration
-------------

The following are configuration options for public use. For options
which are either enabled or disabled, 1 enables and 0 disables.

-   PUMA\_NODEPAGES: Specifies the number of pages to allocate per chunk
    in the per-thread chunk list. Default 2

-   OPENMP: Enable OpenMP. If disabled, we use PUMA’s pthread-based
    thread pooling solution (experimental). Default enabled

-   STATIC\_THREADPOOL: If enabled and we are not using OpenMP, we share
    one thread pool amongst all instances of PUMASet. Default disabled

-   BINDIR: Where we place the build shared library. Default
    {pumadir}/bin

-   VALGRIND: Whether we build with valgrind support. Default enabled

The following are configuration options for use during PUMA development.
They may severely hurt performance so should never be used in
performance-critical code.

-   DEBUG: Enable assertions. Default disabled


Future Work
===========

- MPI support

- Further language bindings (Python, Fortran, C++)

- Windows support

- Better userspace NUMA profiling tooling (could build on Valgrind or LLDB)


API Reference
=============

PUMA Set Management
-------------------

    struct pumaSet* createPumaSet(size_t elementSize, size_t numThreads, char* threadAffinity);

Creates a new `struct pumaSet`.

Arguments:

<span>l X</span> `elementSize` & Size of each element in the set.\
`numThreads` & The number of threads we want to run pumaSet on.\
`threadAffinity` & An affinity string specifying the CPUs to which to
bind threads. Can contain numbers separated either by commas or dashes.
“i-j” means bind to every cpu from i to j inclusive. “i,j” means bind to
i and j. Formats can be mixed: for example, “0-3, 6, 10, 12, 15, 13” is
valid.If `NULL`, binds each thread to the CPU whose number matches the
thread (tid 0 == cpu 0 :: tid 1 == cpu 1 :: etc.).If non-`NULL`, must
specify at least as many CPUs as there are threads.

\

------------------------------------------------------------------------

    void destroyPumaSet(struct pumaSet* set);

Destroys and frees memory from the `struct pumaSet`.\

------------------------------------------------------------------------

    size_t getNumElements(struct pumaSet* set);

Returns the total number of elements in the `struct pumaSet`.\

------------------------------------------------------------------------

    typedef size_t (splitterFunc)(void* perElemBalData, size_t numThreads,
    		void* extraData);

Signature for a function which, given an element, the total number of
threads and, optionally, a void pointer, will specify the thread with
which to associate the element.

Arguments:

<span>l X</span> `perElemBalData` & Per-element data passed into
`pumallocManualBalancing()` which enables the splitter to choose the
placement of the associated element.\
`numThreads` & The total number of threads in use.\
`extraData` & Optional extra data, set by calling
`pumaListSetBalancer()`.

\

------------------------------------------------------------------------

    void pumaSetBalancer(struct pumaSet* set, bool autoBalance, splitterFunc* splitter, void* splitterExtraData);

Sets the balancing strategy for a `struct pumaSet`.

Arguments:

<span>l X</span> `set` & Set to set the balancing strategy for.\
`autoBalance` & Whether to automatically balance the set across threads
prior to each kernel run.\
`splitter` & A pointer to a function which determines the thread with
which to associate new data when `pumallocManualBalancing()` is called.\
`splitterExtraData` & A void pointer to be passed to the splitter
function each time it is called.

Memory Allocation
-----------------

    void* pumalloc(struct pumaSet* set);

Adds an element to the `struct pumaSet` and returns a pointer to it. The
new element is associated with the CPU on which the current thread is
running.\

------------------------------------------------------------------------

    void* pumallocManualBalancing(struct pumaSet* set, void* balData);

Adds an element to the `struct pumaSet` and returns a pointer to it.
Passes `balData` to the set’s splitter function to determine the CPU
with which to associate the new element.\

------------------------------------------------------------------------

    void* pumallocAutoBalancing(struct pumaSet* set);

Adds an element to the `struct pumaSet` and returns a pointer to it.
Automatically associates the new element with the CPU with the fewest
elements.\

------------------------------------------------------------------------

    void pufree(void* element);

Frees the specified element from its set.

Kernel Application
------------------

    struct pumaExtraKernelData
    {
    	void* (*extraDataConstructor)(void* constructorData);
    	void* constructorData;
    	void (*extraDataDestructor)(void* data);
    	void (*extraDataThreadReduce)(void* data);
    	void (*extraDataReduce)(void* retValue, void* data[],
    			unsigned int nThreads);
    	void* retValue;
    };

A descriptor of functions which handle extra data for kernels to pass
into runKernel().

Members:

<span>l X</span> `extraDataConstructor` & A per-thread constructor for
extra data which is passed into the kernel.\
`constructorData` & A pointer to any extra data which may be required by
the constructor. May be `NULL`.\
`extraDataDestructor` & A destructor for data created with
`extraDataConstructor()`.\
`extraDataThreadReduce` & A finalisation function which is run after the
kernel on a per-thread basis. Takes the per-thread data as an argument.\
`extraDataReduce` & A global finalisation function which is run after
all threads have finished running the kernel. Takes retValue, an array
of the extra data for all threads and the number of threads in use.\
`retValue` & A pointer to a return value for use by extraDataReduce. May
be `NULL`.\

\

------------------------------------------------------------------------

    void initKernelData(struct pumaExtraKernelData* kernelData,
    		void* (*extraDataConstructor)(void* constructorData),
    		void* constructorData,
    		void (*extraDataDestructor)(void* data),
    		void (*extraDataThreadReduce)(void* data),
    		void (*extraDataReduce)(void* retValue, void* data[],
    				unsigned int nThreads),
    		void* retValue);

Initialises `kernelData`. Any or all of the arguments after `kernelData`
may be `NULL`. Any `NULL` functions are set to dummy functions which do
nothing.\

------------------------------------------------------------------------

    extern struct pumaExtraKernelData emptyKernelData;

A dummy descriptor for extra kernel data. Causes `NULL` to be passed to
the kernel in place of extra data.\

------------------------------------------------------------------------

    typedef void (*pumaKernel)(void* element, void* extraData);

The type signature for kernels which are to be run on a PUMA list.

Arguments:

<span>l X</span> `element` & The current element in our iteration.\
`extraData` & Extra information specified by our extra data descriptor.\

\

------------------------------------------------------------------------

    void runKernel(struct pumaSet* set, pumaKernel kernel, struct pumaExtraKernelData* extraDataDetails);

Applies the given kernel to all elements in a `struct pumaSet`.

Arguments:

<span>l X</span> `set` & The set containing the elements to which we
want to apply our kernel.\
`kernel` & A pointer to the kernel to apply.\
`extraDataDetails` & A pointer to the structure specifying the extra
data to be passed into the kernel.\

\

------------------------------------------------------------------------

    void runKernelList(struct pumaSet* set, pumaKernel kernels[],
    		size_t numKernels, struct pumaExtraKernelData* extraDataDetails);

Applies the given kernels to all elements in a `struct pumaSet`. Kernels
are applied in the order in which they are specified in the array.

Arguments:

<span>l X</span> `set` & The set containing the elements to which we
want to apply our kernels.\
`kernels` & An array of kernels to apply.\
`numKernels` & The number of kernels to apply.\
`extraDataDetails` & A pointer to the structure specifying the extra
data to be passed into the kernels.\

Static Data Allocation
----------------------

    void* pumallocStaticLocal(size_t size);

Allocates thread-local storage which resides on the NUMA domain to which
the CPU which executes the function belongs.

Arguments:

<span>l X</span> `size` & The number of bytes we want to allocate.

\

------------------------------------------------------------------------

    void* pumaDeleteStaticData(void);

Deletes all static data associated with the current thread.

Getting Started: Standard Deviation Hello World!
================================================

In lieu of the traditional “Hello World” introductory program, we
present a PUMA-based program which generates a large set of random
numbers between 0 and 1 and uses the reduction mechanism of PUMA to
calculate the set’s standard deviation.

In order to calculate the standard deviation, we require three things: a
kernel, a constructor for the per-thread data and a reduction function.
In the constructor, we use the `pumallocStaticLocal()` function to
allocate a static variable on a per-thread basis which resides in memory
local to the core to which each thread is pinned.

This interface for allocating thread-local data are only intended to be
used for static data whose lifespan extends to the end of the program.
It is possible to delete all static data which is related to a thread,
but it is more sensible to simply reuse the allocated memory each time
we need similarly-sized data on a thread. This requires the use of
pthread keys in order to retrieve the allocated pointer each time it is
needed.

    // puma.h contains all of the puma public API declarations we need.
    #include "puma.h"
    #include <math.h>
    #include <pthread.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <getopt.h>

    pthread_key_t extraDataKey;
    pthread_once_t initExtraDataOnce = PTHREAD_ONCE_INIT;

    static void initialiseKey(void)
    {
    	pthread_key_create(&extraDataKey, NULL);
    }

    struct stdDevExtraData
    {
    	double sum;
    	double squareSum;
    	size_t numElements;
    };

    static void* extraDataConstructor(void* constructorData)
    {
    	(void)pthread_once(&initExtraDataOnce, &initialiseKey);

    	void* stdDevExtraData = pthread_getspecific(extraDataKey);

    	if(stdDevExtraData == NULL)
    	{
    		stdDevExtraData = pumallocStaticLocal(sizeof(struct stdDevExtraData));
    		pthread_setspecific(extraDataKey, stdDevExtraData);
    	}

    	return stdDevExtraData;
    }

    static void extraDataReduce(void* voidRet, void* voidData[],
    		unsigned int nThreads)
    {
    	double* ret = (double*)voidRet;
    	double sum = 0;
    	double squareSum = 0;
    	size_t numElements = 0;

    	for(unsigned int i = 0; i < nThreads; ++i)
    	{
    		struct stdDevExtraData* data = (struct stdDevExtraData*)voidData[i];
    		numElements += data->numElements;
    		sum += data->sum;
    		squareSum += data->squareSum;
    	}

    	double mean = sum / numElements;
    	*ret = squareSum / numElements - (mean * mean);
    }

    static void stdDevKernel(void* voidNum, void* voidData)
    {
    	double num = *(double*)voidNum;
    	struct stdDevExtraData* data = (struct stdDevExtraData*)voidData;
    	data->sum += num;
    	data->squareSum += num * num;
    	++data->numElements;
    }

    static void staticDestructor(void* arg)
    {
    	pumaDeleteStaticData();
    }

Prior to running the kernel, we must actually create the
`struct pumaSet` which contains our data; to do this, we specify the
size of our elements, the number of threads we wish to use and,
optionally, a string detailing what cores we want to pin threads to. We
must also seed the random number generator and read the arguments:

    static void printHelp(char* invocationName)
    {
    	printf("Usage: %s -t numThreads -e numElements [-a affinityString]\n"
    		"\tnumThreads: The number of threads to use\n"
    		"\tnumElements: The number of numbers to allocate\n"
    		"\taffinityString: A string which specifies which cores to run on.\n");
    }

    int main(int argc, char** argv)
    {
    	int numThreads = 1;
    	int numElements = 1000;
    	char* affinityStr = NULL;

    	/*
    		Get command line input for the affinity string and number of threads.
    	*/
    	char c;

    	while ( (c = getopt(argc, argv, "e:a:t:")) != -1)
    	{
    		switch (c)
    		{
    			case 't':
    				numThreads = atoi(optarg);
    				break;
    			case 'e':
    				numElements = atoi(optarg);
    				break;
    			case 'a':
    				affinityStr = optArg;
    				break;
    			case 'h':
    				printHelp(argv[0]);
    				break;
    		}
    	}

    	struct pumaSet* set =
    			createPumaSet(sizeof(double), numThreads, affinityStr);
    	srand(time(NULL));

From here, we can use the `pumalloc` call to allocate space within `set`
for each number:

    	for(size_t i = 0; i < numElements; ++i)
    	{
    		double* num = (double*)pumalloc(set);
    		*num = (double)rand() / RAND_MAX;
    	}

We then use `initKernelData()` to create the extra data to be passed
into our kernel. From there, we call `runKernel()` to invoke our kernel
and get the standard deviation of the set.

    	struct pumaExtraKernelData kData;
    	double stdDev = -1;
    	initKernelData(&kData, &extraDataConstructor, NULL, NULL, NULL,
    			&extraDataReduce, &stdDev);
    	runKernel(set, stdDevKernel, &kData);

    	printf("Our set has a standard deviation of %f\n"
    			"Also, Hello World!\n", stdDev);

Finally, we clean up after ourselves by destroying our set and all our
static data. The static data destructor destroys data on a per-thread
basis, so we must call the destructor from all threads in our pool. To
do this, we use the `executeOnThreadPool()` function from
`pumathreadpool.h`.

    	executeOnThreadPool(set->threadPool, staticDestructor, NULL);
    	destroyPumaSet(set);
    }

In order to compile this tutorial, use the following command:

`gcc -pthread -std=c99 <file>.c -o stddev -lpuma -L<PUMA bin dir> -I<PUMA inc dir>`

Licence
=======

PUMA is released under the three-clause BSD licence.
We chose this rather than a copyleft licence like GPL or LGPL in order
to allow anyone to use PUMA with absolute freedom aside from the
inclusion of a short copyright notice.
