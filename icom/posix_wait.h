#ifndef POSIX_WAIT_H
#define POSIX_WAIT_H

#include "posix_thread.h"

typedef struct __waitable_handle {
    int sync_; /* as boolean check */
#if _WIN32
    HANDLE cond_;
#else
    pthread_cond_t cond_;
    pthread_condattr_t condattr_;
    int pass_;
    posix__pthread_mutex_t mutex_;
#endif
} posix__waitable_handle_t;

__extern__
int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter);
__extern__
int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter);
__extern__
void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter);

/*
 * 让一个等待对象执行等待操作 
 * @waiter  等待对象
 * @tsc     等待超时的毫秒级设置
 * 
 * 返回定义:
 * 如果 tsc <= 0 则有:
 * 0: 事件触发
 * -1: 系统调用失败
 * 
 * 如果 tsc > 0 则有：
 * 0: 事件触发
 * ETIMEOUT: 等待超时
 * -1: 系统调用失败 
 */
#if !defined INFINITE
#define INFINITE (0xFFFFFFFF)
#endif
__extern__
int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, uint32_t tsc/*ms*/);

/*
 * 唤醒 @waiter 对象
 * 如果 @waiter 是同步对象， 则使用 @waiter 并处于等待中的线程集合内， 有任意一个线程被唤醒
 * 如果 @waiter 是通告对象， 则使用 @waiter 并处于等待中的线程集合内， 所有线程均被唤醒
 * 如果 @waiter 是通告对象， 则必须显式调用 posix__block_waitable_handle 以设置对象重新阻塞
 */
__extern__
int posix__sig_waitable_handle(posix__waitable_handle_t *waiter);

/*
 * (通告对象)用于重置通过标记
 * 过程内部没有对对象类型进行判断， 同步对象调用此过程的行为不保证正确性
 */
__extern__
void posix__block_waitable_handle(posix__waitable_handle_t *waiter);
__extern__
void posix__reset_waitable_handle(posix__waitable_handle_t *waiter);

#define DECLARE_SYNC_WAITER(name)   \
    struct __waitable_handle name; \
    posix__init_synchronous_waitable_handle(&name)

#define DECLARE_NOTI_WATIER(name) \
    struct __waitable_handle name; \
    posix__init_notification_waitable_handle(&name)

/* 挂起当前线程(死等状态) */
__extern__
void posix__hang();

/* 高精度的固定延时执行 */
__extern__
int posix__delay_execution(uint64_t us);

#endif /* POSIX_WAIT_H */

