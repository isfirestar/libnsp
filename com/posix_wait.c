#include "posix_wait.h"

#include "compiler.h"

/*
class stack of pthread_cond_wait
(gdb) bt
#0  __GI_raise (sig=sig@entry=6) at ../sysdeps/unix/sysv/linux/raise.c:51
#1  0x00007ffff7839c41 in __GI_abort () at abort.c:79
#2  0x00007ffff787af17 in __libc_message (action=action@entry=(do_abort | do_backtrace), fmt=fmt@entry=0x7ffff798027b "%s") at ../sysdeps/posix/libc_fatal.c:181
#3  0x00007ffff787afc2 in __GI___libc_fatal (message=message@entry=0x7ffff7bcd9e0 "The futex facility returned an unexpected error code.") at ../sysdeps/posix/libc_fatal.c:191
#4  0x00007ffff7bc77be in futex_fatal_error () at ../sysdeps/nptl/futex-internal.h:200
#5  futex_wait_cancelable (private=<optimized out>, expected=<optimized out>, futex_word=<optimized out>) at ../sysdeps/unix/sysv/linux/futex-internal.h:105
#6  __pthread_cond_wait_common (abstime=0x0, mutex=0x601080 <mutex>, cond=0x601101 <block+1>) at pthread_cond_wait.c:502
#7  __pthread_cond_wait (cond=0x601101 <block+1>, mutex=0x601080 <mutex>) at pthread_cond_wait.c:655
#8  0x0000000000400803 in main (argc=1, argv=0x7fffffffdea8) at cmain.c:65

man page of futex, importent:
The uaddr argument points to the futex word.  On all platforms,
futexes are four-byte integers that must be aligned on a four-byte
boundary.  The operation to perform on the futex is specified in the
futex_op argument; val is a value whose meaning and purpose depends
on futex_op.

pointer address return by @malloc always aligned of 4 bytes
*/

struct __posix__waitable_handle_t {
#if !_WIN32
    pthread_cond_t cond_;
    pthread_condattr_t condattr_;
    int pass_;
    posix__pthread_mutex_t mutex_;
#endif
};

static
int __posix_init_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
#if !_WIN32
        struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)malloc(sizeof(struct __posix__waitable_handle_t));
        if (!phandle ) {
            return RE_ERROR(ENOMEM);
        }

        posix__pthread_mutex_init(&phandle->mutex_);
        pthread_condattr_init(&phandle->condattr_);
        /* using CLOCK_MONOTONIC time check method */
        pthread_condattr_setclock(&phandle->condattr_, CLOCK_MONOTONIC);
        pthread_cond_init(&phandle->cond_, &phandle->condattr_);
        /* initialize the pass condition */
        phandle->pass_ = 0;

        waiter->handle_ = phandle;
#endif
        return 0;
    }

    return -1;
}

/*--------------------------------------------------------------------------------------------------------------------------*/
int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }

    waiter->sync_ = 1;

#if _WIN32
    waiter->handle_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!waiter->handle_) {
        return -1;
    }
    return 0;
#else
    return __posix_init_waitable_handle(waiter);
#endif
}

int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }
    waiter->sync_ = 0;

#if _WIN32
    waiter->handle_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!waiter->handle_) {
        return -1;
    }
    return 0;
#else
    return __posix_init_waitable_handle(waiter);
#endif
}

/*--------------------------------------------------------------------------------------------------------------------------*/

void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
#if _WIN32
        if (waiter->handle_) {
            CloseHandle(waiter->handle_);
            waiter->handle_ = NULL;
        }
#else
        struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
        if ( phandle ) {
            pthread_condattr_destroy(&phandle->condattr_);
            pthread_cond_destroy(&phandle->cond_);
            posix__pthread_mutex_release(&phandle->mutex_);
            free(phandle);
        }
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
    struct __posix__waitable_handle_t *phandle;
#endif

    if (!waiter) {
        return RE_ERROR(EINVAL);
    }
    retval = 0;

#if _WIN32
    if (!waiter->handle_) {
        return -1;
    }

    if (0 == tsc || tsc < 0x7FFFFFFF) {
        waitRes = WaitForSingleObject(waiter->handle_, tsc);
    } else {
        waitRes = WaitForSingleObject(waiter->handle_, INFINITE);
    }

    if (WAIT_FAILED == waitRes) {
        return -1;
    } else if (WAIT_TIMEOUT == waitRes) {
        return ETIMEDOUT;
    } else {
        return 0;
    }
#else
    phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
    if (!phandle) {
        return RE_ERROR(EINVAL);
    }

    if (0 == tsc || tsc >= 0x7FFFFFFF) {
        posix__pthread_mutex_lock(&phandle->mutex_);

        /* synchronous wait object need to set @pass_ flag to zero befor wait syscall,
           no mater what status it is now */
        if (waiter->sync_) {
            phandle->pass_ = 0;
            while (!phandle->pass_) {
                retval = pthread_cond_wait(&phandle->cond_, &phandle->mutex_.handle_);
                /* fail syscall */
                if (0 != retval) {
                    retval = -1;
                    break;
                }
            }

            /* reset @pass_ flag to zero immediately after wait syscall,
                to maintain semantic consistency with ms-windows-API WaitForSingleObject*/
            phandle->pass_ = 0;
        } else {

            /* for notification waitable handle, 
                all thread blocked on wait method will be awaken by pthread_cond_broadcast(3P)(@posix__sig_waitable_handle)
                the object is always in a state of signal before method @posix__reset_waitable_handle called.
                */
            if (!phandle->pass_) {
                retval = pthread_cond_wait(&phandle->cond_, &phandle->mutex_.handle_);
            }
        }

        posix__pthread_mutex_unlock(&phandle->mutex_);
        return retval;
    }

    /* wait with timeout */
    if (clock_gettime(CLOCK_MONOTONIC, &abstime) >= 0) {
        /* 从当前时间计算延迟，防止64位环境下的纳秒溢出(tv_nsec >= 1000000000 会导致 pthread_cond_timedwait 返回参数错误 ) */
        uint64_t nsec = abstime.tv_nsec;
        nsec += ((uint64_t) tsc * 1000000); /* convert milliseconds to nanoseconds */
        abstime.tv_sec += (nsec / 1000000000);
        abstime.tv_nsec = (nsec % 1000000000);
        posix__pthread_mutex_lock(&phandle->mutex_);

        /* synchronous wait object need to set @pass_ flag to zero befor wait syscall,
           no mater what status it is now */
        if (waiter->sync_) {
            phandle->pass_ = 0;
            while (!phandle->pass_) {
                retval = pthread_cond_timedwait(&phandle->cond_, &phandle->mutex_.handle_, &abstime);
                /* timedout, break the loop */
                if (ETIMEDOUT == retval) {
                    break;
                }

    			/* fail syscall */
                if (0 != retval) {
                    retval = -1;
                    break;
                }
            }
		
            /* reset @pass_ flag to zero immediately after wait syscall,
                to maintain semantic consistency with ms-windows-API WaitForSingleObject*/
            phandle->pass_ = 0;
        } else {
            /* for notification waitable handle, 
                all thread blocked on wait method will be awaken by pthread_cond_broadcast(3P)(@posix__sig_waitable_handle)
                the object is always in a state of signal before method @posix__reset_waitable_handle called.
                */
            if (!phandle->pass_) {
                retval = pthread_cond_timedwait(&phandle->cond_, &phandle->mutex_.handle_, &abstime);
            }
         }
        posix__pthread_mutex_unlock(&phandle->mutex_);
    }
    return retval;
#endif
}

int posix__sig_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return -1;
    }

#if _WIN32
    if (!waiter->handle_) {
        return -1;
    }
    return SetEvent(waiter->handle_);
#else
    struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
    posix__pthread_mutex_lock(&phandle->mutex_);
    phandle->pass_ = 1;
    if (waiter->sync_) {
        pthread_cond_signal(&phandle->cond_);
    } else {
        pthread_cond_broadcast(&phandle->cond_);
    }
    posix__pthread_mutex_unlock(&phandle->mutex_);

    return 0;
#endif
}

void posix__block_waitable_handle(posix__waitable_handle_t *waiter) {
#if _WIN32
    ResetEvent(waiter->handle_);
#else
    if (waiter) {
        struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
        /* @reset operation effect only for notification wait object.  */
        if (!waiter->sync_) {
            posix__pthread_mutex_lock(&phandle->mutex_);
            phandle->pass_ = 0;
            posix__pthread_mutex_unlock(&phandle->mutex_);
        }
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

int posix__delay_execution( uint64_t us ) {
#if _WIN32
	typedef NTSTATUS( WINAPI * DelayExecution )( BOOL bAlertable, PLARGE_INTEGER pTimeOut );
	static DelayExecution ZwDelayExecution = NULL;
	static HINSTANCE inst = NULL;

	if ( !ZwDelayExecution ) {
		if ( !inst ) {
			inst = LoadLibraryA( "ntdll.dll" );
			if ( !inst ) {
				return -1;
			}
		}
		ZwDelayExecution = ( DelayExecution )GetProcAddress( inst, "NtDelayExecution" );
	}
	if ( ZwDelayExecution ) {
		LARGE_INTEGER TimeOut;
		TimeOut.QuadPart = -1 * us * 10;
		if ( !NT_SUCCESS( ZwDelayExecution( FALSE, &TimeOut ) ) ) {
			return -1;
		}
		return 0;
	}

	return -1;
#else
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = us;
	return select(0, NULL, NULL, NULL, &tv);
#endif
}