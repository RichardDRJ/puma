#ifndef _PUMA__INTERNAL__PUMA_DOMAIN_H_
#define _PUMA__INTERNAL__PUMA_DOMAIN_H_

#include <stdlib.h>

struct pumaDomain
{
	size_t numListsInDomain;
	struct pumaThreadList* listsInDomain;
};

int _getCurrentNumaDomain(void);
size_t _getNumDomains(void);
size_t _getNumCPUs(void);
size_t _getCurrentCPUIndexInDomain(void);
size_t _getNumCPUsInDomain(int domain);

#endif // _PUMA__INTERNAL__PUMA_DOMAIN_H_
