#include "internal/numa.h"
#include "internal/pumanode.h"
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

void* numalloc_local(size_t psize)
{
	void* ret;
#if PUMA_NODEPAGES > 1 || !defined(NNUMA)
	int status = posix_memalign(&ret, psize, psize);

#ifndef NNUMA
	numa_setlocal_memory(ret, psize);
#endif // NNUMA
#else
	ret = numa_alloc_local(psize);
#endif

	assert(status == 0 || (printf("status = %d\n", status), false)); (void)status;

	return ret;
}
