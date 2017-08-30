#include "posix_wait.h"

static
int __posix_init_waitable_handle(posix__waitable_handle_t *waitable_handle) {
    if (waitable_handle) {
#if !_WIN32
        posix__pthread_mutex_init(&waitable_handle->mutex_);
        pthread_condattr_init(&waitable_handle->condattr_);
        /*����ʱ����ո�ΪCLOCK_MONOTONIC*/
        pthread_condattr_setclock(&waitable_handle->condattr_, CLOCK_MONOTONIC);
        pthread_cond_init(&waitable_handle->cond_, &waitable_handle->condattr_);
        /*��ʼ���Ź�����*/
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
            /* ϵͳ����ʧ�ܣ� ����ѭ��, �϶�Ϊ����ʧ�� */
            if (0 != retval) {
                retval = -1;
                break;
            }
        }

        /* �����ͬ������ ����Ҫ����ͨ����� */
        if (waiter->sync_) {
            waiter->pass_ = 0;
        }

        posix__pthread_mutex_unlock(&waiter->mutex_);
        return retval;
    }

    /* ����ʱ�� */
    if (clock_gettime(CLOCK_MONOTONIC, &abstime) >= 0) {
        /* �ӵ�ǰʱ������ӳ٣���ֹ64λ�����µ��������(tv_nsec >= 1000000000 �ᵼ�� pthread_cond_timedwait ���ز������� ) */
        uint64_t nsec = abstime.tv_nsec;
        nsec += ((uint64_t) tsc * 1000000); /* ����ת��Ϊ���� */
        abstime.tv_sec += (nsec / 1000000000);
        abstime.tv_nsec = (nsec % 1000000000);
        posix__pthread_mutex_lock(&waiter->mutex_);
        while (!waiter->pass_) {
            retval = pthread_cond_timedwait(&waiter->cond_, &waiter->mutex_.handle_, &abstime);
            /* ��ʱ��ϵͳ����ʧ�ܣ����˳�ѭ���� ϵͳ���óɹ��� ����Ҫ�ж��Ƿ� pass */
            if (ETIMEDOUT == retval) {
                break;
            }

            if (0 != retval) {
                retval = -1;
                break;
            }
        }
        /* �����ͬ������ ����Ҫ����ͨ����� */
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

void posix__reset_waitable_handle(posix__waitable_handle_t *waiter) {
    posix__block_waitable_handle(waiter);
}

void posix__hang() {
    DECLARE_SYNC_WAITER(waiter);
    posix__waitfor_waitable_handle(&waiter, -1);
}

int posix__delay_exec(uint32_t ms) {
#if _WIN32
    static HANDLE timeout = NULL;
    if (!timeout) {
        timeout = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    
    if (!timeout) {
        return -1;
    }
    
    if (WAIT_TIMEOUT != WaitForSingleObject(timeout, ms)) {
        return -1;
    }
    return 0;
#else
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ms * 1000;
    return select(0, NULL, NULL, NULL, &tv);
#endif
}