#ifndef _PUMA__INTERNAL__PUMA_THREAD_POOL_H_
#define _PUMA__INTERNAL__PUMA_THREAD_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

struct pumathreadpool;

#ifdef STATIC_THREADPOOL
struct pumaThreadPool* getThreadPool(void);
void setupThreadPool(size_t numThreads, char* affinityStr);
#else
struct pumaThreadPool* createThreadPool(size_t numThreads, char* affinityStr);
#endif

void executeOnThreadPool(struct pumaThreadPool* tp,
		void (*workFunction)(void* arg), void* arg);
size_t pumaGetThreadNum(void);
void freeThreadPool(struct pumaThreadPool* pool);
size_t pumaGetNumThreads(struct pumaThreadPool* pool);
size_t pumaGetCPUNum(void);
double pumaGetTimeWaitedForPool(void);


#ifdef __cplusplus
}
#endif

#endif // _PUMA__INTERNAL__PUMA_THREAD_POOL_H_
