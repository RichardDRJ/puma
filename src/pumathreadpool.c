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

static pthread_once_t offsetKeyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t timeOffsetKey;

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

	void (*workFunction)(void* arg);
	void* arg;
	double timeSeconds;
};

static pthread_key_t numThreadsKey;
static pthread_key_t threadNumKey;
static pthread_key_t cpuNumKey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

size_t pumaGetThreadNum(void)
{
	void* ptr = pthread_getspecific(threadNumKey);
	if(ptr != NULL)
		return *((size_t*)ptr);
	else
		return 0;
}

size_t pumaGetNumThreads(struct pumaThreadPool* pool)
{
	if(pool == NULL)
		return *((size_t*)pthread_getspecific(numThreadsKey));
	else
		return pool->numThreads;
}

size_t pumaGetCPUNum(void)
{
	void* ptr = pthread_getspecific(cpuNumKey);
	if(ptr != NULL)
		return *((size_t*)ptr);
	else
		return 0;
}

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

void executeOnThreadPool(struct pumaThreadPool* tp,
		void (*workFunction)(void* arg), void* arg)
{
	tp->workFunction = workFunction;
	tp->arg = arg;
	pthread_barrier_wait(&tp->workWaitBarrier);
	pthread_barrier_wait(&tp->doneBarrier);

	tp->timeSeconds = 0;
	for(size_t i = 0; i < tp->numThreads; ++i)
		tp->timeSeconds = _max(tp->threadsInfo[i]->timeSeconds, tp->timeSeconds);

	(void)pthread_once(&offsetKeyOnce, &_makeTimeKey);
	*(double*)pthread_getspecific(timeOffsetKey) += tp->timeSeconds;
}

static void* _threadPoolWorker(void* arg)
{
	struct _threadPoolWorkerInfo* info = (struct _threadPoolWorkerInfo*)arg;
	struct pumaThreadPool* pool = info->pool;

	pthread_setspecific(numThreadsKey, &pool->numThreads);
	pthread_setspecific(threadNumKey, &info->threadNum);
	pthread_setspecific(cpuNumKey, &info->cpu);

#ifdef __linux__
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(info->cpu, &set);
	sched_setaffinity(0, sizeof(set), &set);
#endif

	PROFILING_DECLS(doWork);

	while(true)
	{
		// pthread_mutex_lock(&info->workFunctionMutex)
		// while(info->workFunction == NULL)
		// 	pthread_cond_wait(info->moreWorkCondition, &info->workFunctionMutex);

		pthread_barrier_wait(&pool->workWaitBarrier);
		void (*workFunction)(void* arg) = pool->workFunction;
		void* arg = pool->arg;

		// info->workFunction = NULL:
		// pthread_mutex_unlock(&info->workFunctionMutex)

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

struct pumaThreadPool* newThreadPool(size_t numThreads, char* affinityStr)
{
	struct pumaThreadPool* threadPool =
			(struct pumaThreadPool*)calloc(1, sizeof(struct pumaThreadPool));

	threadPool->numThreads = (numThreads != 0) ? numThreads : sysconf(_SC_NPROCESSORS_ONLN);
	threadPool->threads = (pthread_t*)calloc(numThreads, sizeof(pthread_t));
	threadPool->threadsInfo =
			(struct _threadPoolWorkerInfo**)calloc(numThreads,
					sizeof(struct _threadPoolWorkerInfo*));
	pthread_barrier_init(&threadPool->workWaitBarrier, NULL, numThreads + 1);
	pthread_barrier_init(&threadPool->doneBarrier, NULL, numThreads + 1);

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
#endif
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

	return threadPool;
}

double pumaGetTimeWaitedForPool(void)
{
	void* ptr = pthread_getspecific(timeOffsetKey);
	if(ptr != NULL)
		return *((double*)ptr);
	return 0;
}

void freeThreadPool(struct pumaThreadPool* pool)
{
	for(size_t i = 0; i < pool->numThreads; ++i)
		(void)pthread_cancel(pool->threads[i]);

	free(pool->threads);
	pthread_barrier_destroy(&pool->workWaitBarrier);
	pthread_barrier_destroy(&pool->doneBarrier);
	free(pool);
}

double pumaGetLastExecutionTime(struct pumaThreadPool* pool)
{
	return pool->timeSeconds;
}
