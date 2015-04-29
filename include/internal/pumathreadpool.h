#ifndef _PUMALIST__INTERNAL__PUMA_THREAD_POOL_H_
#define _PUMALIST__INTERNAL__PUMA_THREAD_POOL_H_

#include <stdlib.h>

struct pumathreadpool;

struct pumaThreadPool* _newThreadPool(size_t numThreads, char* affinityStr);
void _executeOnThreadPool(struct pumaThreadPool* tp,
		void (*workFunction)(void* arg), void* arg);
size_t _pumaGetThreadPoolNumber(void);
size_t _pumaGetNumThreadsInPool(void);
void _freeThreadPool(struct pumaThreadPool* pool);
size_t _getThreadPoolNumThreads(struct pumaThreadPool* pool);
size_t _pumaGetCPUNum(void);

#endif // _PUMALIST__INTERNAL__PUMA_THREAD_POOL_H_
