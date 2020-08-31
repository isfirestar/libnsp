#ifndef POSIX_THREAD_H
#define POSIX_THREAD_H

#include "compiler.h"

#if _WIN32

#include <Windows.h>

struct __posix_pthread {
    boolean_t detached_;
    HANDLE pid_;
};

#define POSIX_PTHREAD_TYPE_DECLARE(name)    \
            posix__pthread_t name ={ .pid_ = NULL }
#define POSIX_PTHREAD_TYPE_INIT  { .pid_ = NULL }

struct __posix__pthread_mutex {
    CRITICAL_SECTION handle_;
};

#else /* POSIX */

/* -lpthread */
#include <pthread.h>

struct __posix_pthread {
    boolean_t detached_;
    pthread_t pid_;
    pthread_attr_t attr_;
} __POSIX_TYPE_ALIGNED__;

#define POSIX_PTHREAD_TYPE_DECLARE(name)    \
            posix__pthread_t name ={ .detached_ = NO, .pid_ = 0 }

#define POSIX_PTHREAD_TYPE_INIT \
            {.detached_ = NO, .pid_ = 0 }

struct __posix__pthread_mutex {
    pthread_mutex_t handle_;
} __POSIX_TYPE_ALIGNED__;

#endif /* _WIN32 */

typedef struct __posix_pthread          posix__pthread_t;
typedef struct __posix__pthread_mutex   posix__pthread_mutex_t;

/*
 * posix__pthread_create / posix__pthread_critical_create / posix__pthread_realtime_create
 * implementations to create a Thread(LWP) running on normal/RR(kernel)/FIFO(realtime)priority
 * @tidp both input and output parameter to obtain the thread object when success call.
 * @start_rtn : thread execute routine
 * @arg : argument pass to thread function
 */
__interface__
int posix__pthread_create(posix__pthread_t *tidp, void*(*start_rtn)(void*), void *arg);
__interface__
int posix__pthread_self(posix__pthread_t *tidp);
__interface__
int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg);
__interface__
int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg);

/* set the affinity of CPU-core mark by @mask and thread(LWP) specified by @tidp */
__interface__
int posix__pthread_setaffinity(const posix__pthread_t *tidp, int mask);
__interface__
int posix__pthread_getaffinity(const posix__pthread_t *tidp, int *mask);

/* posix__pthread_detach implemenation detach the thread and object @tidp, after detach, the object pointer by @tidp are no longer usable.
 * posix__pthread_joinable examine whether the thread is in detached states or not,  return -1 when detached， otherwise return >=0
 * posix__pthread_join waitting for the thread end and than join the object pointer.
 */
__interface__
int posix__pthread_detach(posix__pthread_t * tidp);
__interface__
boolean_t posix__pthread_joinable(posix__pthread_t * tidp);
__interface__
int posix__pthread_join(posix__pthread_t * tidp, void **retval);

#if _WIN32
#define posix__pthread_exit(exit_code)
#else
#define posix__pthread_exit(exit_code) pthread_exit(exit_code)
#endif

__interface__
int posix__pthread_mutex_init(posix__pthread_mutex_t *mutex);
__interface__
void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex);
__interface__
int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex);

/* try to get lock in @expires milliseconds
 * WIN32 programing not support
 */
__interface__
int posix__pthread_mutex_timedlock(posix__pthread_mutex_t *mutex, uint32_t expires);
__interface__
void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex);
__interface__
void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex);
#define posix__pthread_mutex_uninit(mutex) posix__pthread_mutex_release(mutex)

/* Give up the current thread execution initiative
 * this implementation can interrupte the thread running with @SCHED_FIFO priority.
 *  */
__interface__
void posix__pthread_yield();

#endif /* POSIX_THREAD_H */

