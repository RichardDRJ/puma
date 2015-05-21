#ifndef _PUMALIST__INTERNAL__PUMANODE_H_
#define _PUMALIST__INTERNAL__PUMANODE_H_

#include "internal/bitmask.h"
#include "internal/pumaheader.h"

#ifndef PUMA_NODEPAGES
#define PUMA_NODEPAGES 2
#endif

extern size_t pumaPageSize;

struct pumaNode
{
	struct pumaHeader blockHeader;
	size_t elementSize;

	size_t firstSkipIndex;
	size_t elemsPerPageUnit;
	size_t pageUnit;

	size_t blockSize;
	size_t numPages;

	char* elementArray;
	size_t numElements;
	size_t capacity;
	size_t firstKnownFree;

	struct pumaNode* next;
	struct pumaNode* prev;

	struct pumaBitmask freeMask;
	bool dirty;

	bool active;

	size_t index;

	struct pumaThreadList* threadList;
};

void _cleanupNode(struct pumaNode* currentNode, struct pumaThreadList* list);
struct pumaNode* _appendPumaNode(struct pumaThreadList* threadList,
		size_t elementSize);
void _freePumaNode(struct pumaNode* node);


#endif // _PUMALIST__INTERNAL__PUMANODE_H_
