#ifndef _PUMALIST__NUMA_H_
#define _PUMALIST__NUMA_H_

#ifndef NNUMA
#include <numa.h>
#include <numaif.h>
#endif

#include <stdlib.h>
void* numalloc_local(size_t psize);
void* numalloc_on_node(size_t psize, int domain);
void nufree(void* ptr, size_t size);
void numa_bind_to_node(size_t currDomain);

#endif // _PUMALIST__NUMA_H_
