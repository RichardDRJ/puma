#ifndef _PUMALIST__NUMA_H_
#define _PUMALIST__NUMA_H_

#ifndef NNUMA
#include <numa.h>
#include <numaif.h>
#endif

#include <stdlib.h>
void* numalloc_local(size_t psize);
void nufree(void* ptr, size_t size);

#endif // _PUMALIST__NUMA_H_
