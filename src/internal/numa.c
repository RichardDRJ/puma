#include "internal/numa.h"
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

void* numalloc_local(size_t psize)
{
	void* ret;

	int status = posix_memalign(&ret, psize, psize);

#ifndef NNUMA
	numa_setlocal_memory(ret, psize);
#endif

	assert(status == 0 || (printf("status = %d\n", status), false)); (void)status;

	return ret;
}
