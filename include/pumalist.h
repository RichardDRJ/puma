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

typedef size_t (splitterFunc)(void* balData, size_t numThreads, void* extraData);

struct pumaList
{
	size_t elementSize;

	struct pumaDomain* domains;
	size_t numDomains;
	struct pumaThreadList* threadLists;
	size_t numCores;
	size_t numThreads;

	size_t* threadListToIndex;

	bool autoBalance;
	splitterFunc* splitter;
	void* splitterExtraData;

	struct pumaThreadPool* threadPool;
};

struct pumaList* createPumaList(size_t elementSize, size_t numThreads,
		char* threadAffinity);
void destroyPumaList(struct pumaList* list);

void getPerThreadNumElements(struct pumaList* list, size_t numElements[]);
size_t getNumElements(struct pumaList* list);
size_t getNumElementsMatcher(struct pumaList* list,
		bool (*matcher)(void* element, void* extraData), void* extraData);
void getPerThreadNumNodes(struct pumaList* list, size_t* numNodes);
void pumaListSetBalancer(struct pumaList* list, bool autoBalance,
		splitterFunc* splitter, void* splitterExtraData);

#ifdef __cplusplus
}
#endif

#endif // __PUMALIST__PUMALIST_H__
