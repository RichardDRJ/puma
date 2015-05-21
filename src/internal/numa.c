#include "internal/numa.h"
#include "internal/pumanode.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <errno.h>

static void* bind_to_domain(void* ptr, size_t size, int domain)
{
#if !defined(NNUMA)
	int maxnode = numa_max_node();

	if(maxnode > 0)
	{
		size_t numBuckets = (1 + maxnode / sizeof(unsigned long));
		unsigned long nodemask[numBuckets];
		memset(nodemask, 0, numBuckets * sizeof(unsigned long));

		size_t bucket = domain / sizeof(unsigned long);
		size_t i = domain % sizeof(unsigned long);

		nodemask[bucket] = (unsigned long)1 << ((sizeof(unsigned long) * 8) - (i + 1));

		long status = mbind(ptr, size, MPOL_BIND, nodemask, maxnode,
				MPOL_MF_STRICT | MPOL_MF_MOVE);

		assert(status == 0 || (printf("errno: %d\n", errno), false)); (void)status;
	}
#endif

	(void)ptr;(void)size;(void)domain;
}

void* numalloc_local(size_t psize)
{
	void* ret;
#if PUMA_NODEPAGES > 1 || defined(NNUMA)
	int status = posix_memalign(&ret, psize, psize);

#ifndef NNUMA
	// numa_setlocal_memory(ret, psize);
	int domain = _getCurrentNumaDomain();
	bind_to_domain(ret, psize, domain);
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
	bind_to_domain(ret, psize, domain);
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
