#ifndef _PUMA__NUMA_H_
#define _PUMA__NUMA_H_

#ifndef NNUMA
#include <numa.h>
#include <numaif.h>
#endif

#include <stdlib.h>
void* numalloc_aligned_on_node(size_t psize, int domain);
void* numalloc_on_node(size_t psize, int domain);
void nufree(void* ptr, size_t size);
void nufree_aligned(void* ptr, size_t size);

#endif // _PUMA__NUMA_H_
