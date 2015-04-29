#define _GNU_SOURCE
#include <sched.h>

#include "internal/pumadomain.h"
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
#ifdef NNUMA
	return sysconf(_SC_NPROCESSORS_ONLN);
#else
	return numa_num_task_cpus();
#endif
}

size_t _getNumCPUsInDomain(int domain)
{
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
