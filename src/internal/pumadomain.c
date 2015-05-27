#define _GNU_SOURCE
#include <sched.h>

#include "internal/pumadomain.h"
#include "internal/pumautil.h"
#include "internal/pumathreadlist.h"
#include "internal/numa.h"
#include "pumathreadpool.h"

#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

int _getCurrentNumaDomain(void)
{
#ifdef NNUMA
	int node = 0;
#else
	int cpu = pumaGetCPUNum();
	int node = numa_node_of_cpu(cpu);
#endif

	return node;
}

size_t _getCurrentCPUIndexInDomain(void)
{
#ifdef NNUMA
	size_t index = pumaGetThreadNum();
#else
	int cpu = pumaGetCPUNum();

	int node = numa_node_of_cpu(cpu);

	struct bitmask* cpumask = numa_allocate_cpumask();

	int status = numa_node_to_cpus(node, cpumask);
	assert(status == 0); (void)status;

	unsigned long originalSize = cpumask->size;
	cpumask->size = cpu;

	size_t index = numa_bitmask_weight(cpumask);

	cpumask->size = originalSize;

	numa_free_cpumask(cpumask);
#endif

	return index;
}

size_t _getNumDomains(void)
{
#ifdef NNUMA
	return 1;
#else
	return numa_num_configured_nodes();
#endif
}

size_t _getNumCPUs(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

size_t _getNumCPUsInDomain(int domain)
{
	(void)domain;
#ifdef NNUMA
	return _getNumCPUs();
#else
	struct bitmask* cpumask = numa_allocate_cpumask();
	int status = numa_node_to_cpus(domain, cpumask);
	assert(status == 0); (void)status;

	size_t index = numa_bitmask_weight(cpumask);

	numa_free_cpumask(cpumask);

	return index;
#endif
}

void destroyDomain(struct pumaDomain* domain)
{
	size_t domNumCores = domain->numListsInDomain;
	size_t domListSize =
			_getSmallestContainingPages(domNumCores * sizeof(struct pumaThreadList));
	nufree(domain->listsInDomain, domListSize);
}

void initDomain(struct pumaDomain* domain, size_t domainNumber)
{
	size_t domNumCores = _getNumCPUsInDomain(domainNumber);
	domain->numListsInDomain = domNumCores;
	size_t domListSize =
			_getSmallestContainingPages(domNumCores * sizeof(struct pumaThreadList));
	domain->listsInDomain = numalloc_on_node(domListSize, domainNumber);

	for(size_t i = 0; i < domNumCores; ++i)
	{
		VALGRIND_MAKE_MEM_NOACCESS(domain->listsInDomain + i,
				sizeof(struct pumaThreadList));
	}
}
