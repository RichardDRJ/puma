#include "internal/numa.h"
#include "internal/pumadomain.h"
#include "internal/pumanode.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <errno.h>

void* numalloc_on_node(size_t psize, int domain)
{
	void* ret;

	int status = posix_memalign(&ret, psize, psize);
	assert(status == 0 || (printf("status = %d\n", status), false)); (void)status;

#ifndef NNUMA
	int maxnode = numa_max_node();
	size_t numBuckets = (1 + maxnode / sizeof(unsigned long));
	unsigned long nodemask[numBuckets];

	memset(nodemask, 0, numBuckets * sizeof(unsigned long));

	size_t bucket = domain / sizeof(unsigned long);
	size_t i = domain % sizeof(unsigned long);

	nodemask[bucket] = (unsigned long)1 << ((sizeof(unsigned long) * 8) - (i + 1));

	size_t numPages = psize / pumaPageSize;

	/*	Fault the pages in. */
	for(size_t p = 0; p < numPages; ++p)
		*1(char*)(ret + p * pumaPageSize) = 0;

	mbind(ret, psize, MPOL_BIND, nodemask, maxnode + 1,
			MPOL_MF_STRICT | MPOL_MF_MOVE);

#endif // NNUMA
	

	assert(ret == (struct pumaNode*)((size_t)ret & ~((pumaPageSize * PUMA_NODEPAGES) - 1)));

	return ret;
}

void nufree(void* ptr, size_t size)
{
	free(ptr);
}
