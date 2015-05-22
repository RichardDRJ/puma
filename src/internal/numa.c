#include "internal/numa.h"
#include "internal/pumadomain.h"
#include "internal/pumanode.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <errno.h>

#include <sys/mman.h>

void* numalloc_on_node(size_t psize, int domain)
{
	void* ret = mmap(NULL, psize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

#ifndef NNUMA
	int maxnode = numa_max_node();
	size_t numBuckets = (1 + maxnode / sizeof(unsigned long));
	unsigned long nodemask[numBuckets];

	memset(nodemask, 0, numBuckets * sizeof(unsigned long));

	size_t bucket = domain / sizeof(unsigned long);
	size_t i = domain % sizeof(unsigned long);

	nodemask[bucket] = (unsigned long)1 << i;


	int status = mbind(ret, psize, MPOL_BIND, nodemask, maxnode + 2,
			MPOL_MF_STRICT | MPOL_MF_MOVE);

	assert(status == 0 || (printf("mbind failed: %d\n", errno), false));
	(void)status;
#endif // NNUMA
	

	assert(ret == (struct pumaNode*)((size_t)ret & ~((pumaPageSize * PUMA_NODEPAGES) - 1)));

	return ret;
}

void* numalloc_aligned_on_node(size_t psize, int domain)
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

	nodemask[bucket] = (unsigned long)1 << i;


	status = mbind(ret, psize, MPOL_BIND, nodemask, maxnode + 2,
			MPOL_MF_STRICT | MPOL_MF_MOVE);

	assert(status == 0 || (printf("mbind failed: %d\n", errno), false));
#endif // NNUMA
	

	assert(ret == (struct pumaNode*)((size_t)ret & ~((pumaPageSize * PUMA_NODEPAGES) - 1)));

	return ret;
}

void nufree(void* ptr, size_t size)
{
	munmap(ptr, size);
}

void nufree_aligned(void* ptr, size_t size)
{
	free(ptr);
}
