#include "internal/pumathreadlist.h"
#include "internal/pumanode.h"
#include "pumalist.h"
#include "internal/pumadomain.h"
#include "internal/valgrind.h"

#include <stdlib.h>

struct pumaThreadList* _getListForCurrentThread(struct pumaList* list)
{
	int domainNumber = _getCurrentNumaDomain();
	struct pumaDomain* d = list->domains + domainNumber;

	size_t cpuIndex = _getCurrentCPUIndexInDomain();

	struct pumaThreadList* listsInDomain = d->listsInDomain;
	struct pumaThreadList* tl = listsInDomain + cpuIndex;
	return tl;
}

void _emptyThreadList(struct pumaThreadList* tl)
{
	struct pumaNode* node = tl->head;

	while(node != NULL)
	{
		struct pumaNode* nextNode = node->next;
		_freePumaNode(node);
		node = nextNode;
	}
}