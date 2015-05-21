#include "internal/numa.h"
#include "internal/pumanode.h"
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

void* numalloc_local(size_t psize)
{
	void* ret;
#if PUMA_NODEPAGES > 1 || defined(NNUMA)
	int status = posix_memalign(&ret, psize, psize);

#ifndef NNUMA
	numa_setlocal_memory(ret, psize);
#endif // NNUMA
	
	assert(status == 0 || (printf("status = %d\n", status), false)); (void)status;
#else
	ret = numa_alloc_local(psize);
#endif

	assert(ret == (struct pumaNode*)((size_t)ret & ~((pumaPageSize * PUMA_NODEPAGES) - 1)));

	return ret;
}

void nufree(void* ptr, size_t size)
{
#if PUMA_NODEPAGES > 1 || defined(NNUMA)
	free(ptr);
#else
	numa_free(ptr, size);
#endif
}

void numa_bind_to_node(size_t currDomain)
{
#ifndef NNUMA
	int maxNode = numa_max_node();

	size_t numBuckets = (1 + maxNode / sizeof(unsigned long));
	unsigned long nodemask[numBuckets];
	memset(nodemask, 0, numBuckets * sizeof(unsigned long));

	size_t bucket = currDomain / sizeof(unsigned long);
	size_t i = currDomain % sizeof(unsigned long);

	nodemask[bucket] = 1 << ((sizeof(unsigned long) * 8) - (i + 1));

	set_mempolicy(MPOL_BIND, nodemask, maxNode);
#endif // NNUMA
}