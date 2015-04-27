#include "internal/pumathreadlist.h"
#include "pumalist.h"
#include "internal/pumadomain.h"
#include "internal/valgrind.h"

struct pumaThreadList* _getListForCurrentThread(struct pumaList* list)
{
	int domainNumber = _getCurrentNumaDomain();
	struct pumaDomain* d = list->domains + domainNumber;

	size_t cpuIndex = _getCurrentCPUIndexInDomain();

	struct pumaThreadList* listsInDomain = d->listsInDomain;
	struct pumaThreadList* tl = listsInDomain + cpuIndex;
	return tl;
}
