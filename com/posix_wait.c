#include "posix_wait.h"

static
int __posix_init_waitable_handle(posix__waitable_handle_t *waitable_handle) {
    if (waitable_handle) {
#if !_WIN32
        posix__pthread_mutex_init(&waitable_handle->mutex_);
        pthread_condattr_init(&waitable_handle->condattr_);
        /*测量时间参照改为CLOCK_MONOTONIC*/
        pthread_condattr_setclock(&waitable_handle->condattr_, CLOCK_MONOTONIC);
        pthread_cond_init(&waitable_handle->cond_, &waitable_handle->condattr_);
        /*初始化放过条件*/
        waitable_handle->pass_ = 0;
#endif
        return 0;
    }

    return -1;
}

/*--------------------------------------------------------------------------------------------------------------------------*/
int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return -EINVAL;
    }

    waiter->sync_ = 1;

#if _WIN32
    waiter->cond_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!waiter->cond_) {
        return -1;
    }
#else
    if (__posix_init_waitable_handle(waiter) < 0) {
        return -EINVAL;
    }
#endif
    return 0;
}

int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return -EINVAL;
    }
    waiter->sync_ = 0;

#if _WIN32
    waiter->cond_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!waiter->cond_) {
        return -1;
    }
#else
    if (__posix_init_waitable_handle(waiter) < 0) {
        return -EINVAL;
    }
#endif
    return 0;
}

/*--------------------------------------------------------------------------------------------------------------------------*/

void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
#if _WIN32
        if (waiter->cond_) {
            CloseHandle(waiter->cond_);
            waiter->cond_ = NULL;
        }
#else
        pthread_condattr_destroy(&waiter->condattr_);
        pthread_cond_destroy(&waiter->cond_);
        posix__pthread_mutex_release(&waiter->mutex_);
#endif
    }
}

int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, uint32_t tsc) {
    int retval;
#if _WIN32
    DWORD waitRes;
#else
    /* -D_POSIX_C_SOURCE >= 199703L */
    struct timespec abstime;
#endif

    if (!waiter) {
        return -EINVAL;
    }
    retval = 0;

#if _WIN32
    if (!waiter->cond_) {
        return -1;
    }

    if (0 == tsc || tsc < 0x7FFFFFFF) {
        waitRes = WaitForSingleObject(waiter->cond_, tsc);
    } else {
        waitRes = WaitForSingleObject(waiter->cond_, INFINITE);
    }

    if (WAIT_FAILED == waitRes) {
        return -1;
    } else if (WAIT_TIMEOUT == waitRes) {
        return ETIMEDOUT;
    } else {
        return 0;
    }
#else
    if (0 == tsc || tsc >= 0x7FFFFFFF) {
        posix__pthread_mutex_lock(&waiter->mutex_);
        while (!waiter->pass_) {
            retval = pthread_cond_wait(&waiter->cond_, &waiter->mutex_.handle_);
            /* 系统调用失败， 跳出循环, 认定为返回失败 */
            if (0 != retval) {
                retval = -1;
                break;
            }
        }

        /* 如果是同步对象， 则需要重置通过标记 */
        if (waiter->sync_) {
            waiter->pass_ = 0;
        }

        posix__pthread_mutex_unlock(&waiter->mutex_);
        return retval;
    }

    /* 带超时等 */
    if (clock_gettime(CLOCK_MONOTONIC, &abstime) >= 0) {
        /* 从当前时间计算延迟，防止64位环境下的纳秒溢出(tv_nsec >= 1000000000 会导致 pthread_cond_timedwait 返回参数错误 ) */
        uint64_t nsec = abstime.tv_nsec;
        nsec += ((uint64_t) tsc * 1000000); /* 毫秒转换为纳秒 */
        abstime.tv_sec += (nsec / 1000000000);
        abstime.tv_nsec = (nsec % 1000000000);
        posix__pthread_mutex_lock(&waiter->mutex_);
        while (!waiter->pass_) {
            retval = pthread_cond_timedwait(&waiter->cond_, &waiter->mutex_.handle_, &abstime);
            /* 超时或系统调用失败，均退出循环， 系统调用成功， 则需要判定是否 pass */
            if (ETIMEDOUT == retval) {
                break;
            }

            if (0 != retval) {
                retval = -1;
                break;
            }
        }
        /* 如果是同步对象， 则需要重置通过标记 */
        if (waiter->sync_) {
            waiter->pass_ = 0;
        }

        posix__pthread_mutex_unlock(&waiter->mutex_);
    }
    return retval;
#endif
}

int posix__sig_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return -1;
    }

#if _WIN32
    if (!waiter->cond_) {
        return -1;
    }
    return SetEvent(waiter->cond_);
#else
    posix__pthread_mutex_lock(&waiter->mutex_);
    waiter->pass_ = 1;
    if (waiter->sync_) {
        pthread_cond_signal(&waiter->cond_);
    } else {
        pthread_cond_broadcast(&waiter->cond_);
    }
    posix__pthread_mutex_unlock(&waiter->mutex_);

    return 0;
#endif
}

void posix__block_waitable_handle(posix__waitable_handle_t *waiter) {
#if _WIN32
    ResetEvent(waiter->cond_);
#else
    if (waiter) {
        posix__pthread_mutex_lock(&waiter->mutex_);
        waiter->pass_ = 0;
        posix__pthread_mutex_unlock(&waiter->mutex_);
    }
#endif
}

void posix__reset_waitable_handle(posix__waitable_handle_t *waiter){
    posix__block_waitable_handle(waiter);
}

void posix__hang() {
    DECLARE_SYNC_WAITER(waiter);
    posix__waitfor_waitable_handle(&waiter, -1);
}