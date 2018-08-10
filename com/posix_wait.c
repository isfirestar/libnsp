#include "compiler.h"
#include "posix_wait.h"

/*--------------------------------------------------------------------------------------------------------------------------*/
#if _WIN32

int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }

    waiter->sync_ = 1;
    waiter->handle_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!waiter->handle_) {
        return -1;
    }
    return 0;
}

int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }

    waiter->sync_ = 0;
    waiter->handle_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!waiter->handle_) {
        return -1;
    }
    return 0;
}

void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
        if (waiter->handle_) {
            CloseHandle(waiter->handle_);
            waiter->handle_ = NULL;
        }
    }
}

int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, uint32_t tsc) {
    DWORD waitRes;

    if (!waiter) {
        return RE_ERROR(EINVAL);
    }
    
    if (!waiter->handle_) {
        return -1;
    }

    /* if t state of the specified object is signaled before wait function called, the return value willbe @WAIT_OBJECT_0
        either synchronous event or notification event.*/
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
}

int posix__sig_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }

    if (!waiter->handle_) {
        return RE_ERROR(EINVAL);
    }
    return SetEvent(waiter->handle_);
}

void posix__block_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
        if (waiter->handle_ && waiter->sync_ == 0) {
            ResetEvent(waiter->handle_);
        }
    }
}

int posix__delay_execution( uint64_t us ) {
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
}

#else  // _WIN32

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

man page of futex(7), importent:
The uaddr argument points to the futex word.  On all platforms,
futexes are four-byte integers that must be aligned on a four-byte boundary.
The operation to perform on the futex is specified in the futex_op argument;
val is a value whose meaning and purpose depends on futex_op.

notes:
pointer address return by @malloc always aligned to 4 bytes
*/

struct __posix__waitable_handle_t {
    pthread_cond_t cond_;
    pthread_condattr_t condattr_;
    int pass_;
    posix__pthread_mutex_t mutex_;
    int pending;
};

static
int __posix_init_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
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
        /* initialize the pending count of this object */
        phandle->pending = 0;

        waiter->handle_ = phandle;
        return 0;
    }

    return -1;
}

int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }

    waiter->sync_ = 1;
    return __posix_init_waitable_handle(waiter);
}

int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }
    waiter->sync_ = 0;
    return __posix_init_waitable_handle(waiter);
}

void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
        struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
        if ( phandle ) {
            /* object destory routine will be blocked when the waitting reference count greater then zero
                destory routine awaken the threads which waitting for this object */

            pthread_condattr_destroy(&phandle->condattr_);
            pthread_cond_destroy(&phandle->cond_);
            posix__pthread_mutex_release(&phandle->mutex_);
            free(phandle);
        }
    }
}

int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, uint32_t tsc) {
    int retval;
    struct timespec abstime; /* -D_POSIX_C_SOURCE >= 199703L */
    struct __posix__waitable_handle_t *phandle;

    if (!waiter) {
        return RE_ERROR(EINVAL);
    }
    
    phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
    if (!phandle) {
        return RE_ERROR(EINVAL);
    }

    retval = 0;
    if (0 == tsc || tsc >= 0x7FFFFFFF) {
        posix__pthread_mutex_lock(&phandle->mutex_);

        /* increase the pending count of this object to protect self memory */
        ++phandle->pending;

        if (waiter->sync_) {
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

        /* decase the pending count when wait method completed */
        --phandle->pending;

        posix__pthread_mutex_unlock(&phandle->mutex_);
        return retval;
    }

    /* wait with timeout */
    retval = clock_gettime(CLOCK_MONOTONIC, &abstime);
    if (0 == retval ) {
        /* Calculation delay from current time，if tv_nsec >= 1000000000 will cause pthread_cond_timedwait EINVAL, 64 bit overflow */
        uint64_t nsec = abstime.tv_nsec;
        nsec += ((uint64_t) tsc * 1000000); /* convert milliseconds to nanoseconds */
        abstime.tv_sec += (nsec / 1000000000);
        abstime.tv_nsec = (nsec % 1000000000);

        posix__pthread_mutex_lock(&phandle->mutex_);

        /* increase the pending count of this object to protect self memory */
        ++phandle->pending;

        if (waiter->sync_) {
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

         /* decase the pending count when wait method completed */
        --phandle->pending;

        posix__pthread_mutex_unlock(&phandle->mutex_);
        return retval;
    }
    
    return RE_ERROR(errno);
}

int posix__sig_waitable_handle(posix__waitable_handle_t *waiter) {
    if (!waiter) {
        return RE_ERROR(EINVAL);
    }

    struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
    if (!phandle) {
        return RE_ERROR(EINVAL);
    }

    posix__pthread_mutex_lock(&phandle->mutex_);
    phandle->pass_ = 1;
    if (waiter->sync_) {
        pthread_cond_signal(&phandle->cond_);
    } else {
        pthread_cond_broadcast(&phandle->cond_);
    }
    posix__pthread_mutex_unlock(&phandle->mutex_);
    return 0;
}

void posix__block_waitable_handle(posix__waitable_handle_t *waiter) {
    if (waiter) {
        struct __posix__waitable_handle_t *phandle = (struct __posix__waitable_handle_t *)waiter->handle_;
        /* @reset operation effect only for notification wait object.  */
        if (phandle && 0 == waiter->sync_) {
            posix__pthread_mutex_lock(&phandle->mutex_);
            phandle->pass_ = 0;
            posix__pthread_mutex_unlock(&phandle->mutex_);
        }
    }
}

int posix__delay_execution( uint64_t us ) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = us;
    return select(0, NULL, NULL, NULL, &tv);
}

#endif

/*--------------------------------------------------------------------------------------------------------------------------*/
void posix__reset_waitable_handle(posix__waitable_handle_t *waiter) {
    posix__block_waitable_handle(waiter);
}

void posix__hang() {
    DECLARE_SYNC_WAITER(waiter);
    posix__waitfor_waitable_handle(&waiter, -1);
}