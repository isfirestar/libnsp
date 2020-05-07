#include "compiler.h"
#include "posix_wait.h"

/*--------------------------------------------------------------------------------------------------------------------------*/
#if _WIN32

static
int __posix_init_waitable_handle(posix__waitable_handle_t *waiter)
{
	if (!waiter) {
		return -EINVAL;
	}

	waiter->cond_ = CreateEvent(NULL, waiter->sync_ ? FALSE : TRUE, FALSE, NULL);
	if (!waiter->cond_) {
		return posix__makeerror(GetLastError());
	}

	return 0;
}

int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter)
{
    if (!waiter) {
        return -EINVAL;
    }

    waiter->sync_ = 1;
    return __posix_init_waitable_handle(waiter);
}

int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter)
{
    if (!waiter) {
        return -EINVAL;
    }

    waiter->sync_ = 0;
	return __posix_init_waitable_handle(waiter);
}

void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter)
{
    if (waiter) {
		if (waiter->cond_) {
			CloseHandle(waiter->cond_);
			waiter->cond_ = NULL;
        }
    }
}

int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, int interval)
{
    DWORD waitRes;

    if (!waiter) {
        return -EINVAL;
    }

	if (!waiter->cond_) {
        return -EBADF;
    }

    /* if t state of the specified object is signaled before wait function called, the return value willbe @WAIT_OBJECT_0
        either synchronous event or notification event.*/
    if (interval >= 0) {
		waitRes = WaitForSingleObject(waiter->cond_, (DWORD)interval);
    } else {
		waitRes = WaitForSingleObject(waiter->cond_, INFINITE);
    }

    if (WAIT_FAILED == waitRes) {
		return posix__makeerror(GetLastError());
    } else if (WAIT_TIMEOUT == waitRes) {
        return ETIMEDOUT;
    } else {
        return 0;
    }
}

int posix__sig_waitable_handle(posix__waitable_handle_t *waiter)
{
    if (!waiter) {
        return -EINVAL;
    }

	if (!waiter->cond_) {
        return -EBADF;
    }

	return SetEvent(waiter->cond_);
}

void posix__block_waitable_handle(posix__waitable_handle_t *waiter)
{
    if (waiter) {
		if (waiter->cond_ && waiter->sync_ == 0) {
			ResetEvent(waiter->cond_);
        }
    }
}

int posix__delay_execution( uint64_t us )
{
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

#else  /* POSIX */

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

static
int __posix_init_waitable_handle(posix__waitable_handle_t *waiter)
{
    int retval;
    pthread_condattr_t condattr;

    if (!waiter) {
        return -EINVAL;
    }

    /* waitable handle MUST locked by a internal mutex object */
    retval = posix__pthread_mutex_init(&waiter->mutex_);
    if (retval < 0) {
        return retval;
    }

    pthread_condattr_init(&condattr);
    do {
        /* using CLOCK_MONOTONIC time check method */
        retval = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
        if (0 != retval) {
            retval = posix__makeerror(retval);
            break;
        }

        /* OK, initial the condition variable now */
        retval = pthread_cond_init(&waiter->cond_, &condattr);
        if (0 != retval) {
            retval = posix__makeerror(retval);
            break;
        }

        /* initialize the pass condition */
        waiter->pass_ = 0;
        pthread_condattr_destroy(&condattr);
        return 0;
    } while (0);

    posix__pthread_mutex_release(&waiter->mutex_);
    pthread_condattr_destroy(&condattr);
    return retval;
}

int posix__init_synchronous_waitable_handle(posix__waitable_handle_t *waiter)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(waiter);

    waiter->sync_ = 1;
    return __posix_init_waitable_handle(waiter);
}

int posix__init_notification_waitable_handle(posix__waitable_handle_t *waiter)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(waiter);

    waiter->sync_ = 0;
    return __posix_init_waitable_handle(waiter);
}

void posix__uninit_waitable_handle(posix__waitable_handle_t *waiter)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_NR__(waiter);
    pthread_cond_destroy(&waiter->cond_);
    posix__pthread_mutex_release(&waiter->mutex_);
}

static
int __posix__infinite_waitfor_waitable_handle(posix__waitable_handle_t *waiter)
{
    int retval;

    assert(waiter);

    retval = 0;

    posix__pthread_mutex_lock(&waiter->mutex_);
    if (waiter->sync_) {
        while (!waiter->pass_) {
            if (0 != (retval = pthread_cond_wait(&waiter->cond_, &waiter->mutex_.handle_))) {
                break;
            }
        }

        /* reset @pass_ flag to zero immediately after wait syscall,
            to maintain semantic consistency with ms-windows-API WaitForSingleObject*/
        waiter->pass_ = 0;
    } else {

        /* for notification waitable handle,
            all thread blocked on wait method will be awaken by pthread_cond_broadcast(3P)(@posix__sig_waitable_handle)
            the object is always in a state of signal before method @posix__reset_waitable_handle called.
            */
        if (!waiter->pass_) {
            retval = pthread_cond_wait(&waiter->cond_, &waiter->mutex_.handle_);
        }
    }

    posix__pthread_mutex_unlock(&waiter->mutex_);
    return posix__makeerror(retval);
}

int posix__waitfor_waitable_handle(posix__waitable_handle_t *waiter, int interval)
{
    int retval;
    struct timespec abstime; /* -D_POSIX_C_SOURCE >= 199703L */
    uint64_t nsec;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(waiter);

    /* the waiter using infinite wait model */
    if (interval <= 0) {
        return __posix__infinite_waitfor_waitable_handle(waiter);
    }

    /* wait with timeout */
    if (0 != clock_gettime(CLOCK_MONOTONIC, &abstime)) {
         return posix__makeerror(errno);
    }

    /* Calculation delay from current time，if tv_nsec >= 1000000000 will cause pthread_cond_timedwait EINVAL, 64 bit overflow */
    nsec = abstime.tv_nsec;
    nsec += ((uint64_t) interval * 1000000); /* convert milliseconds to nanoseconds */
    abstime.tv_sec += (nsec / 1000000000);
    abstime.tv_nsec = (nsec % 1000000000);

    retval = 0;

    posix__pthread_mutex_lock(&waiter->mutex_);

    if (waiter->sync_) {
        while (!waiter->pass_) {
            retval = pthread_cond_timedwait(&waiter->cond_, &waiter->mutex_.handle_, &abstime);
            if (0 != retval) { /* timedout or fatal syscall cause the loop break */
                break;
            }
        }

        /* reset @pass_ flag to zero immediately after wait syscall,
            to maintain semantic consistency with ms-windows-API WaitForSingleObject*/
        waiter->pass_ = 0;
    } else {
        /* for notification waitable handle,
            all thread blocked on wait method will be awaken by pthread_cond_broadcast(3P)(@posix__sig_waitable_handle)
            the object is always in a state of signal before method @posix__reset_waitable_handle called.
            */
        if (!waiter->pass_) {
            retval = pthread_cond_timedwait(&waiter->cond_, &waiter->mutex_.handle_, &abstime);
        }
    }

    posix__pthread_mutex_unlock(&waiter->mutex_);

    if (0 != retval) {
        if (ETIMEDOUT != retval) {
            retval = posix__makeerror(retval);
        }
    }

    return retval;
}

int posix__sig_waitable_handle(posix__waitable_handle_t *waiter)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(waiter);

    posix__pthread_mutex_lock(&waiter->mutex_);
    waiter->pass_ = 1;
    if (waiter->sync_) {
        retval = pthread_cond_signal(&waiter->cond_);
    } else {
        retval = pthread_cond_broadcast(&waiter->cond_);
    }
    posix__pthread_mutex_unlock(&waiter->mutex_);

    return posix__makeerror(retval);
}

void posix__block_waitable_handle(posix__waitable_handle_t *waiter)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_NR__(waiter);

     /* @reset operation effect only for notification wait object.  */
    if ( 0 == waiter->sync_) {
        posix__pthread_mutex_lock(&waiter->mutex_);
        waiter->pass_ = 0;
        posix__pthread_mutex_unlock(&waiter->mutex_);
    }
}

int posix__delay_execution( uint64_t us )
{
    int fdset;
    struct timeval tv;

    tv.tv_sec = us / 1000000;
    tv.tv_usec = us % 1000000;

    fdset = select(0, NULL, NULL, NULL, &tv);
    if (fdset < 0) {
        return posix__makeerror(errno);
    }

    return 0;
}

#endif

/*--------------------------------------------------------------------------------------------------------------------------*/
int posix__allocate_synchronous_waitable_handle(posix__waitable_handle_t **waiter)
{
    posix__waitable_handle_t *inner;
    int retval;

    inner = (posix__waitable_handle_t *)malloc(sizeof(posix__waitable_handle_t));
    if (!inner) {
        return -ENOMEM;
    }
    inner->sync_ = 1;

    retval = __posix_init_waitable_handle(inner);
    if ( retval < 0) {
        free(inner);
        return retval;
    }

    if (waiter) {
        *waiter = inner;
    }
    return 0;
}

int posix__allocate_notification_waitable_handle(posix__waitable_handle_t **waiter)
{
    posix__waitable_handle_t *inner;
    int retval;

    inner = (posix__waitable_handle_t *)malloc(sizeof(posix__waitable_handle_t));
    if (!inner) {
        return -ENOMEM;
    }
    inner->sync_ = 0;

    retval = __posix_init_waitable_handle(inner);
    if ( retval < 0) {
        free(inner);
        return retval;
    }

    if (waiter) {
        *waiter = inner;
    }
    return 0;
}

void posix__release_waitable_handle(posix__waitable_handle_t *waiter)
{
    if (waiter) {
        posix__uninit_waitable_handle(waiter);
        free(waiter);
    }
}

void posix__reset_waitable_handle(posix__waitable_handle_t *waiter)
{
    posix__block_waitable_handle(waiter);
}

void posix__hang()
{
    DECLARE_SYNC_WAITER(waiter);
    posix__waitfor_waitable_handle(&waiter, -1);
}
