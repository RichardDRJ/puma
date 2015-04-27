#ifndef __PUMALIST__PUMALIST_H__
#define __PUMALIST__PUMALIST_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

struct pumaThreadList;
struct pumaDomain;
struct pumaList;

struct pumaList
{
	size_t elementSize;

	struct pumaDomain* domains;
	size_t numDomains;
	struct pumaThreadList* threadLists;
	size_t numCores;
};

struct pumaList* createPumaList(size_t elementSize);
void destroyPumaList(struct pumaList* list);

void getPerThreadNumElements(struct pumaList* list, size_t numElements[]);
size_t getNumElements(struct pumaList* list);
size_t getNumElementsMatcher(struct pumaList* list,
		bool (*matcher)(void* element, void* extraData), void* extraData);
void getPerThreadNumNodes(struct pumaList* list, size_t* numNodes);

#endif // __PUMALIST__PUMALIST_H__
