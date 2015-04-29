#ifndef _PUMALIST__INTERNAL__PUMA_THREAD_POOL_H_
#define _PUMALIST__INTERNAL__PUMA_THREAD_POOL_H_

#include <stdlib.h>

struct pumathreadpool;

struct pumaThreadPool* newThreadPool(size_t numThreads, char* affinityStr);
void executeOnThreadPool(struct pumaThreadPool* tp,
		void (*workFunction)(void* arg), void* arg);
size_t pumaGetThreadNum(void);
void freeThreadPool(struct pumaThreadPool* pool);
size_t pumaGetNumThreads(struct pumaThreadPool* pool);
size_t pumaGetCPUNum(void);

#endif // _PUMALIST__INTERNAL__PUMA_THREAD_POOL_H_
