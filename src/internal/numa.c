#include "internal/numa.h"
#include "internal/pumanode.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <errno.h>

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

void* numalloc_on_node(size_t psize, int domain)
{
	void* ret;
#if PUMA_NODEPAGES > 1 || defined(NNUMA)
	int status = posix_memalign(&ret, psize, psize);

#ifndef NNUMA
	numa_tonode_memory(ret, psize, domain);
#endif // NNUMA
	
	assert(status == 0 || (printf("status = %d\n", status), false)); (void)status;
#else
	ret = numa_alloc_onnode(psize, domain);
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
#if !defined(NNUMA) && 0
	int maxNode = numa_max_node();

	size_t numBuckets = (1 + maxNode / sizeof(uint64_t));
	uint64_t nodemask[numBuckets];
	memset(nodemask, 0, numBuckets * sizeof(uint64_t));

	size_t bucket = currDomain / sizeof(uint64_t);
	size_t i = currDomain % sizeof(uint64_t);

	nodemask[bucket] = (uint64_t)1 << ((sizeof(uint64_t) * 8) - (i + 1));

	long status = set_mempolicy(MPOL_BIND, nodemask, maxNode);
	assert(status == 0 || (printf("errno: %d\n", errno), false)); (void)status;
#endif // NNUMA
}
