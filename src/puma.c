#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>

#include "puma.h"
#include "internal/pumadomain.h"

#include <stdio.h>
#include <omp.h>
#include <stdbool.h>

extern bool _pinnedWithPumaInit;

void initPuma(int numThreads)
{
	if(numThreads > 0)
		omp_set_num_threads(numThreads);

#ifdef __linux__
	#pragma omp parallel
	{
		int tid = omp_get_thread_num();
		cpu_set_t set;
		CPU_ZERO(&set);
		CPU_SET(tid, &set);
		sched_setaffinity(0, sizeof(set), &set);
	}

	_pinnedWithPumaInit = true;
#endif
}
