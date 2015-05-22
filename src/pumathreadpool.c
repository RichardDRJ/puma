#define _GNU_SOURCE
#include <sched.h>

#include "pumathreadpool.h"
#include "internal/profiling.h"
#include "internal/pthreadbarrier.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifndef NOOPENMP
#include <omp.h>
#endif

#ifdef NOOPENMP
static pthread_once_t offsetKeyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t timeOffsetKey;
#endif // NOOPENMP

#ifdef STATIC_THREADPOOL
static bool _setupComplete = false;
static struct pumaThreadPool* threadPool;
#endif

struct _threadPoolWorkerInfo
{
	struct pumaThreadPool* pool;
	size_t cpu;
	size_t threadNum;
	double timeSeconds;
};

struct pumaThreadPool
{
	size_t numThreads;
	pthread_t* threads;

	struct _threadPoolWorkerInfo** threadsInfo;

	pthread_barrier_t workWaitBarrier;
	pthread_barrier_t doneBarrier;

	pthread_rwlock_t workFunctionLock;

	void (*workFunction)(void* arg);
	void* arg;
	double timeSeconds;
};

#ifdef NOOPENMP
static pthread_key_t numThreadsKey;
static pthread_key_t threadNumKey;
static pthread_key_t cpuNumKey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
#endif // NOOPENMP

size_t pumaGetThreadNum(void)
{
#ifdef NOOPENMP
	void* ptr = pthread_getspecific(threadNumKey);
	if(ptr != NULL)
		return *((size_t*)ptr);
	else
		return 0;
#else
	return omp_get_thread_num();
#endif // NOOPENMP
}

size_t pumaGetNumThreads(struct pumaThreadPool* pool)
{
	if(pool == NULL)
	{
#ifdef NOOPENMP
		return *((size_t*)pthread_getspecific(numThreadsKey));
#else
		return omp_get_max_threads();
#endif // NOOPENMP
	}
	else
		return pool->numThreads;
}

size_t pumaGetCPUNum(void)
{
#ifdef NOOPENMP
	void* ptr = pthread_getspecific(cpuNumKey);
	if(ptr != NULL)
		return *((size_t*)ptr);
	else
		return 0;
#else
	return omp_get_thread_num();
#endif // NOOPENMP
}

#ifdef NOOPENMP
#ifdef __linux__
static void _parseAffinityStr(char* affinityStr, cpu_set_t* set)
{
	char* saveptr;
	char* str = affinityStr;
	char* token;
	CPU_ZERO(set);

	while(token = strtok_r(str, ",", &saveptr))
	{
		str = NULL;
		unsigned int start;
		unsigned int end;
		int matched = sscanf(token, "%u-%u", &start, &end);

		if(matched == 1)
		{
			CPU_SET(start, set);
		}
		else if(matched == 2)
		{
			for(unsigned int i = start; i <= end; ++i)
				CPU_SET(i, set);
		}
		else
		{
			fprintf(stderr, "Error parsing affinity string!\n");
			exit(-1);
		}
	}
}
#endif

static inline double _max(double a, double b)
{
	return (a > b) * a + (a <= b) * b;
}

static void _makeTimeKey(void)
{
	pthread_key_create(&timeOffsetKey, NULL);
	pthread_setspecific(timeOffsetKey, malloc(sizeof(double)));
	*(double*)pthread_getspecific(timeOffsetKey) = 0;
}
#endif // NOOPENMP

void executeOnThreadPool(struct pumaThreadPool* tp,
		void (*workFunction)(void* arg), void* arg)
{
#ifdef NOOPENMP
	pthread_rwlock_wrlock(&tp->workFunctionLock);
	tp->workFunction = workFunction;
	tp->arg = arg;
	pthread_barrier_wait(&tp->workWaitBarrier);
	pthread_rwlock_unlock(&tp->workFunctionLock);
	pthread_barrier_wait(&tp->doneBarrier);

	tp->timeSeconds = 0;
	for(size_t i = 0; i < tp->numThreads; ++i)
		tp->timeSeconds = _max(tp->threadsInfo[i]->timeSeconds, tp->timeSeconds);

	(void)pthread_once(&offsetKeyOnce, &_makeTimeKey);
	*(double*)pthread_getspecific(timeOffsetKey) += tp->timeSeconds;
#else
	#pragma omp parallel num_threads(tp->numThreads)
	{
		workFunction(arg);
	}
#endif // NOOPENMP
}

#ifdef NOOPENMP
static void* _threadPoolWorker(void* arg)
{
	struct _threadPoolWorkerInfo* info = (struct _threadPoolWorkerInfo*)arg;
	struct pumaThreadPool* pool = info->pool;

	/*	Local stack copy because each pthreads thread has its own stack and we
		don't do cross-thread stack writes, meaning that the stack's pages will
		be numa domain local due to first-touch paging.
	*/
	size_t numThreads = pool->numThreads;
	size_t threadNum = info->threadNum;
	size_t cpu = info->cpu;

	pthread_setspecific(numThreadsKey, &numThreads);
	pthread_setspecific(threadNumKey, &threadNum);
	pthread_setspecific(cpuNumKey, &cpu);

#ifdef __linux__
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(info->cpu, &set);
	sched_setaffinity(0, sizeof(set), &set);
#endif

	PROFILING_DECLS(doWork);

	while(true)
	{
		pthread_barrier_wait(&pool->workWaitBarrier);
		pthread_rwlock_rdlock(&pool->workFunctionLock);
		void (*workFunction)(void* arg) = pool->workFunction;
		void* arg = pool->arg;
		pthread_rwlock_unlock(&pool->workFunctionLock);

		PROFILE(doWork, workFunction(arg);)

		info->timeSeconds = GET_ELAPSED_S(doWork);

		pthread_barrier_wait(&pool->doneBarrier);
	}
}

static void _make_keys(void)
{
	pthread_key_create(&numThreadsKey, NULL);
	pthread_key_create(&threadNumKey, NULL);
	pthread_key_create(&cpuNumKey, NULL);
}
#endif // NOOPENMP

#ifdef STATIC_THREADPOOL
struct pumaThreadPool* getThreadPool(void)
{
	if(!_setupComplete)
		fprintf(stderr, "Please call setupThreadPool() before getThreadPool().");

	return threadPool;
}

void setupThreadPool(size_t numThreads, char* affinityStr)
{
	if(_setupComplete)
		return;
#else
struct pumaThreadPool* createThreadPool(size_t numThreads, char* affinityStr)
{
	struct pumaThreadPool* threadPool;
#endif
	threadPool = (struct pumaThreadPool*)calloc(1, sizeof(struct pumaThreadPool));

	numThreads = (numThreads != 0) ? numThreads : sysconf(_SC_NPROCESSORS_ONLN);
	threadPool->numThreads = numThreads;
	threadPool->threads = (pthread_t*)calloc(numThreads, sizeof(pthread_t));

#ifdef NOOPENMP
	threadPool->threadsInfo =
			(struct _threadPoolWorkerInfo**)calloc(numThreads,
					sizeof(struct _threadPoolWorkerInfo*));
	pthread_barrier_init(&threadPool->workWaitBarrier, NULL, numThreads + 1);
	pthread_barrier_init(&threadPool->doneBarrier, NULL, numThreads + 1);
	pthread_rwlock_init(&threadPool->workFunctionLock, NULL);

	(void)pthread_once(&key_once, &_make_keys);

	size_t currThread = 0;

#ifdef __linux__
	if(affinityStr != NULL)
	{
		cpu_set_t cpus;
		CPU_ZERO(&cpus);
		_parseAffinityStr(affinityStr, &cpus);

		size_t nCPUs = CPU_COUNT(&cpus);

		for(size_t i = 0; i < nCPUs && currThread < numThreads; ++i)
		{
			if(CPU_ISSET(i, &cpus))
			{
				struct _threadPoolWorkerInfo* tpInfo =
						(struct _threadPoolWorkerInfo*)malloc(
								sizeof(struct _threadPoolWorkerInfo));
				tpInfo->pool = threadPool;
				tpInfo->cpu = i;
				tpInfo->threadNum = currThread;

				threadPool->threadsInfo[currThread] = tpInfo;

				pthread_create(&threadPool->threads[currThread++], NULL,
						&_threadPoolWorker, tpInfo);
			}
		}
	}
	else
#endif // __linux__
	{
		for(size_t i = 0; currThread < numThreads; ++i)
		{
			struct _threadPoolWorkerInfo* tpInfo =
					(struct _threadPoolWorkerInfo*)malloc(
							sizeof(struct _threadPoolWorkerInfo));
			tpInfo->pool = threadPool;
			tpInfo->cpu = i;
			tpInfo->threadNum = currThread;

			threadPool->threadsInfo[currThread] = tpInfo;

			pthread_create(&threadPool->threads[currThread++], NULL,
					&_threadPoolWorker, tpInfo);
		}
	}
#else
	omp_set_num_threads(numThreads);

	#pragma omp parallel
	{
#ifdef __linux__
		cpu_set_t set;
		CPU_ZERO(&set);
		CPU_SET(omp_get_thread_num(), &set);
		sched_setaffinity(0, sizeof(set), &set);
#endif
	}
#endif // NOOPENMP

#ifdef STATIC_THREADPOOL
	_setupComplete = true;
#else
	return threadPool;
#endif
}

double pumaGetTimeWaitedForPool(void)
{
#ifdef NOOPENMP
	void* ptr = pthread_getspecific(timeOffsetKey);
	if(ptr != NULL)
		return *((double*)ptr);
#endif // NOOPENMP
	return 0;
}

void freeThreadPool(struct pumaThreadPool* pool)
{
	for(size_t i = 0; i < pool->numThreads; ++i)
		(void)pthread_cancel(pool->threads[i]);

#ifdef NOOPENMP
	pthread_barrier_destroy(&pool->workWaitBarrier);
	pthread_barrier_destroy(&pool->doneBarrier);
#endif // NOOPENMP
	free(pool->threads);
	free(pool);
}
