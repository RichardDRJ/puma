#ifndef _PROFILING_H
#define _PROFILING_H

#ifdef NPROFILE
	#define PROFILING_DECLS(__name)

	#define GET_ELAPSED_S(__name) 0.0

	#define GET_STATES(__name)

	#define PROFILE(__name, block) block

	#define PROFILE_PREFIX(__name)

	#define PROFILE_SUFFIX(__name)

#else

	#ifdef WITH_PCM
		#define PCM_PROFILING_DECLS(__name)										\
				SystemCounterState __name##BeforeSState, __name##AfterSState;

		#define PCM_PROFILE_PREFIX(__name)										\
				__name##BeforeSState = getSystemCounterState();

		#define PCM_PROFILE_SUFFIX(__name)										\
				__name##AfterSState = getSystemCounterState();

		#define PCM_GET_STATES(__name) __name##BeforeSState, __name##AfterSState

	#else
		#define PCM_PROFILING_DECLS(__name)

		#define PCM_PROFILE_PREFIX(__name)

		#define PCM_PROFILE_SUFFIX(__name)

		#define PCM_GET_STATES(__name) ERROR //
	#endif

	#include <sys/time.h>
	#include <unistd.h>

	#if defined(__MACH__)	
		#include <mach/mach_init.h>
		#include <mach/thread_act.h>
		#include <mach/mach_port.h>
		#include <assert.h>

		#define PROFILING_DECLS(__name) mach_port_t __name##thread;				\
				kern_return_t __name##kr;										\
				mach_msg_type_number_t __name##count = THREAD_BASIC_INFO_COUNT;	\
				thread_basic_info_data_t __name##start;							\
				thread_basic_info_data_t __name##end;							\
				double __name##EndOffset;										\
				double __name##StartOffset;										\
				PCM_PROFILING_DECLS(__name)

		#define PROFILE_PREFIX(__name)											\
				__name##StartOffset = pumaGetTimeWaitedForPool();				\
				__name##thread = mach_thread_self();							\
				__name##kr = thread_info(__name##thread, THREAD_BASIC_INFO,		\
						(thread_info_t)&__name##start, &__name##count);			\
				assert(__name##kr == KERN_SUCCESS);								\
				PCM_PROFILE_PREFIX(__name)

		#define PROFILE_SUFFIX(__name)											\
				PCM_PROFILE_SUFFIX(__name)										\
				__name##kr = thread_info(__name##thread, THREAD_BASIC_INFO,		\
						(thread_info_t)&__name##end, &__name##count);			\
				assert(__name##kr == KERN_SUCCESS);								\
				__name##EndOffset = pumaGetTimeWaitedForPool();					\
				mach_port_deallocate(mach_task_self(), __name##thread);


		#define GET_ELAPSED_S(__name)											\
				((__name##end.user_time.seconds -								\
				__name##start.user_time.seconds) +								\
				(((__name##end.user_time.microseconds + 1000000) -				\
				__name##start.user_time.microseconds) % 1000000) / 1000000.0) +	\
				(__name##EndOffset - __name##StartOffset)

	#elif defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
		#include <time.h>
		#include "pumathreadpool.h"

		#define PROFILING_DECLS(__name)											\
				struct timespec __name##Start, __name##End;						\
				double __name##EndOffset;										\
				double __name##StartOffset;										\
				PCM_PROFILING_DECLS(__name);

		#define PROFILE_PREFIX(__name)											\
				__name##StartOffset = pumaGetTimeWaitedForPool();				\
				clock_gettime(CLOCK_MONOTONIC, &__name##Start);					\
				PCM_PROFILE_PREFIX(__name);

		#define PROFILE_SUFFIX(__name)											\
				PCM_PROFILE_SUFFIX(__name)										\
				clock_gettime(CLOCK_MONOTONIC, &__name##End);					\
				__name##EndOffset = pumaGetTimeWaitedForPool();


		#define GET_ELAPSED_S(__name)											\
				(__name##End.tv_sec - __name##Start.tv_sec) +					\
				(__name##End.tv_nsec - __name##Start.tv_nsec) / 1000000000.0 +	\
				(__name##EndOffset - __name##StartOffset)
	#endif

	#define GET_STATES(__name) PCM_GET_STATES(__name)

	#define PROFILE(__name, block)												\
			PROFILE_PREFIX(__name)												\
			block																\
			PROFILE_SUFFIX(__name)
#endif

#endif
