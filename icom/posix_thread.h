/* gcc -lpthread */

#ifndef POSIX_THREAD_H
#define POSIX_THREAD_H

#include "compiler.h"

#if _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

typedef struct {
    posix__boolean_t detached_;
#if _WIN32
    HANDLE pid_;
#else
    pthread_t pid_;
    pthread_attr_t attr_;
#endif
} posix__pthread_t;

#if _WIN32
#define POSIX_PTHREAD_TYPE_DECLARE(name)    \
            posix__pthread_t name ={ .pid_ = NULL }
#define POSIX_PTHREAD_TYPE_INIT  { .pid_ = NULL }
#else
#define POSIX_PTHREAD_TYPE_DECLARE(name)    \
            posix__pthread_t name ={ .detached_ = posix__false, .pid_ = 0 }

#define POSIX_PTHREAD_TYPE_INIT \
            {.detached_ = posix__false, .pid_ = 0 }
#endif

typedef struct {
#if _WIN32
    CRITICAL_SECTION handle_;
#else
    pthread_mutex_t handle_;
    pthread_mutexattr_t attr_;
#endif
} posix__pthread_mutex_t;

/*
 * posix__pthread_create / posix__pthread_critical_create / posix__pthread_realtime_create
 * 分别创建普通/RR（核心）/FIFO（实时）优先级别的线程
 * @tidp 线程对象指针
 * @start_rtn 线程过程函数
 * @arg 线程参数
 */
__extern__
int posix__pthread_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg);
__extern__
int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg);
__extern__
int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg);

/* posix__pthread_detach 分离线程和 posix__pthread_t 对象， 分离后对象不可用
 * posix__pthread_joinable 检查线程对象是否处于分离状态， 分离返回-1， 否则返回>=0
 * posix__pthread_join 等待线程结束
 * @tidp 线程对象
 * @retval 线程结束返回值 */
__extern__
int posix__pthread_detach(posix__pthread_t * tidp);
__extern__
posix__boolean_t posix__pthread_joinable(posix__pthread_t * tidp);
__extern__
int posix__pthread_join(posix__pthread_t * tidp, void **retval);

#if _WIN32
#define posix__pthread_exit(exit_code)
#else
#define posix__pthread_exit(exit_code) pthread_exit(exit_code)
#endif

__extern__
void posix__pthread_mutex_init(posix__pthread_mutex_t *mutex);
__extern__
void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex);
__extern__
int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex);
__extern__
void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex);
__extern__
void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex);
#define posix__pthread_mutex_uninit(mutex) posix__pthread_mutex_release(mutex)

/* 主动放弃当前线程执行权
 * 可以主动打断优先级为 SCHED_FIFO 的线程
 *  */
__extern__
void posix__pthread_yield();

#endif /* POSIX_THREAD_H */

