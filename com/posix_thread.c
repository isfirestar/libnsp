#include "posix_thread.h"

#if _WIN32

#include <windows.h>

struct WIN32_THPAR {
    void*(*start_rtn_)(void*);
    void *arg_;
};

uint32_t WINAPI ThProc(void* parameter) {
    struct WIN32_THPAR *par = (struct WIN32_THPAR *) parameter;
    void*(*start_rtn)(void*) = par->start_rtn_;
    void *arg = par->arg_;
    free(par);
    if (start_rtn) {
        return (uint32_t)start_rtn(arg);
    }
    return 0;
}

int posix__pthread_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
    HANDLE th;
    struct WIN32_THPAR *thpar = malloc(sizeof ( struct WIN32_THPAR));
    thpar->arg_ = arg;
    thpar->start_rtn_ = start_rtn;
    th = CreateThread(NULL, 0, ThProc, thpar, 0, NULL);
    if (!th) {
        return -1;
    }

    tidp->detached_ = posix__false;
    tidp->pid_ = th;
    return 0;
}

int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
    HANDLE th;
    struct WIN32_THPAR *thpar = malloc(sizeof ( struct WIN32_THPAR));

    if (!tidp) {
        return RE_ERROR(EINVAL);
    }

    thpar->arg_ = arg;
    thpar->start_rtn_ = start_rtn;
    th = CreateThread(NULL, 0, ThProc, thpar, 0, NULL);
    if (!th) {
        return -1;
    }
    SetThreadPriority(th, THREAD_PRIORITY_TIME_CRITICAL);

    tidp->detached_ = posix__false;
    tidp->pid_ = th;
    return 0;
}

int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
    HANDLE th;
    struct WIN32_THPAR *thpar = malloc(sizeof ( struct WIN32_THPAR));

    if (!tidp) {
        return -EINVAL;
    }

    thpar->arg_ = arg;
    thpar->start_rtn_ = start_rtn;
    th = CreateThread(NULL, 0, ThProc, thpar, 0, NULL);
    if (!th) {
        return -1;
    }

    if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        SetThreadPriority(th, 31);
    }

    tidp->detached_ = posix__false;
    tidp->pid_ = th;
    return 0;
}

int posix__pthread_detach(posix__pthread_t * tidp) {
    if (!tidp) {
        return -EINVAL;
    }

    if (tidp->pid_) {
        CloseHandle(tidp->pid_);
    }

    tidp->pid_ = NULL;
    tidp->detached_ = posix__true;
    return 0;
}

int posix__pthread_join(posix__pthread_t *tidp, void **retval) {
    if (tidp) {
        if (posix__pthread_joinable(tidp)) {
            WaitForSingleObject(tidp->pid_, INFINITE);
			if (retval) {
				DWORD exitCode;
				GetExitCodeThread(tidp->pid_, &exitCode);
				*retval = (void *)exitCode;
			}
			CloseHandle(tidp->pid_);
        }
    }
    return 0;
}

void posix__pthread_mutex_init(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        InitializeCriticalSection(&mutex->handle_);
    }
}

void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        EnterCriticalSection(&mutex->handle_);
    }
}

int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        return convert_boolean_condition_to_retval(TryEnterCriticalSection(&mutex->handle_));
    }
    return -1;
}

int posix__pthread_mutex_timedlock(posix__pthread_mutex_t *mutex, uint32_t expires) {
    return -1;
}

void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        LeaveCriticalSection(&mutex->handle_);
    }
}

void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        DeleteCriticalSection(&mutex->handle_);
    }
}

void posix__pthread_yield() {
    SwitchToThread();
}

#else

#include <unistd.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/types.h>

int posix__pthread_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
    tidp->detached_ = posix__false;
    pthread_attr_init(&tidp->attr_);
    return pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
}

int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
    int retval;
    if (!tidp) {
        return RE_ERROR(EINVAL);
    }

    pthread_attr_init(&tidp->attr_);
    if (pthread_attr_setschedpolicy(&tidp->attr_, SCHED_RR) < 0) {
        return RE_ERROR(errno);
    }

    tidp->detached_ = posix__false;
    retval = pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
    if (0 == retval) {
        return 0;
    }
    return RE_ERROR(retval);
}

int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
    int retval;
    if (!tidp) {
        return RE_ERROR(EINVAL);
    }

    pthread_attr_init(&tidp->attr_);

    /* raiseup policy for realtime thread */
    if (0 == nice(-5)) {
        retval = pthread_attr_setschedpolicy(&tidp->attr_, SCHED_FIFO);
        if ( 0 != retval) {
            return RE_ERROR(retval);
        }
    }

    tidp->detached_ = posix__false;

    retval = pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
    if (0 == retval) {
        return 0;
    }
    return RE_ERROR(retval);
}

int posix__pthread_detach(posix__pthread_t * tidp) {
    if (!tidp) {
        return RE_ERROR(EINVAL);
    }

    if (!posix__pthread_joinable(tidp)) {
        return -1;
    }

    if (pthread_attr_setdetachstate(&tidp->attr_, PTHREAD_CREATE_DETACHED) == 0) {
        pthread_attr_destroy(&tidp->attr_);
        tidp->detached_ = posix__true;
        tidp->pid_ = 0;
        return 0;
    }

    return -1;
}

int posix__pthread_join(posix__pthread_t *tidp, void **retval) {
    if (!tidp) {
        return RE_ERROR(EINVAL);
    }
    if (tidp->pid_ > 0) {
        if (posix__pthread_joinable(tidp)) {
            pthread_join(tidp->pid_, retval);
            pthread_attr_destroy(&tidp->attr_);
        }
    }
    tidp->pid_ = 0;
    return 0;
}

void posix__pthread_mutex_init(posix__pthread_mutex_t *mutex) {
    pthread_mutexattr_init(&mutex->attr_);
    pthread_mutexattr_settype(&mutex->attr_, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex->handle_, &mutex->attr_);
}

void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        pthread_mutex_lock(&mutex->handle_);
    }
}

int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        return pthread_mutex_trylock(&mutex->handle_);
    }
    return -1;
}

int posix__pthread_mutex_timedlock(posix__pthread_mutex_t *mutex, uint32_t expires) {
    struct timespec abstime;
    if (clock_gettime(CLOCK_REALTIME, &abstime) >= 0) {
        uint64_t nsec = abstime.tv_nsec;
        nsec += ((uint64_t) expires * 1000000); /* convert milliseconds to nanoseconds */
        abstime.tv_sec += (nsec / 1000000000);
        abstime.tv_nsec = (nsec % 1000000000);
        return pthread_mutex_timedlock(&mutex->handle_, &abstime);
    }
    return -1;
}

void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        pthread_mutex_unlock(&mutex->handle_);
    }
}

void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex) {
    if (mutex) {
        pthread_mutexattr_destroy(&mutex->attr_);
        pthread_mutex_destroy(&mutex->handle_);
    }
}

void posix__pthread_yield() {
    syscall(__NR_sched_yield);
}

#endif

int posix__pthread_joinable(posix__pthread_t * tidp) {
    return ((!tidp->detached_) && (tidp->pid_ > 0));
}