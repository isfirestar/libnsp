#include "posix_thread.h"

#if !_WIN32
#include <unistd.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/types.h>
#endif

#if _WIN32

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
        start_rtn(arg);
    }
    return 0;
}
#endif

int posix__pthread_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
#if _WIN32
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
#else
    tidp->detached_ = posix__false;
    pthread_attr_init(&tidp->attr_);
    return pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
#endif
}

int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
#if _WIN32
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
    SetThreadPriority(th, THREAD_PRIORITY_TIME_CRITICAL);

    tidp->detached_ = posix__false;
    tidp->pid_ = th;
    return 0;
#else
    if (!tidp) {
        return -EINVAL;
    }

    pthread_attr_init(&tidp->attr_);
    if (pthread_attr_setschedpolicy(&tidp->attr_, SCHED_RR) < 0) {
        return -1;
    }

    tidp->detached_ = posix__false;
    return pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
#endif
}

int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg) {
#if _WIN32
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
        ;
    }

    tidp->detached_ = posix__false;
    tidp->pid_ = th;
    return 0;
#else
    if (!tidp) {
        return -EINVAL;
    }

    pthread_attr_init(&tidp->attr_);

    /* 实时线程直接导致进程优先级提升 */
    if (0 == nice(-5)) {
        if (pthread_attr_setschedpolicy(&tidp->attr_, SCHED_FIFO) < 0) {
            return -1;
        }
    }

    tidp->detached_ = posix__false;
    return pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
#endif
}

int posix__pthread_detach(posix__pthread_t * tidp) {
#if _WIN32
    if (!tidp) {
        return -EINVAL;
    }

    if (tidp->pid_) {
        CloseHandle(tidp->pid_);
    }

    tidp->pid_ = NULL;
    tidp->detached_ = posix__true;
    return 0;
#else
    if (!tidp) {
        return -EINVAL;
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
#endif
}

int posix__pthread_joinable(posix__pthread_t * tidp) {
    return ((!tidp->detached_) && (tidp->pid_ > 0));
}

int posix__pthread_join(posix__pthread_t *tidp, void **retval) {
#if _WIN32
    UNREFERENCED_PARAMETER(retval);
    if (tidp) {
        if (posix__pthread_joinable(tidp)) {
            WaitForSingleObject(tidp->pid_, INFINITE);
            CloseHandle(tidp->pid_);
        }
    }
#else
    if (!tidp) {
        return -EINVAL;
    }
    if (tidp->pid_ > 0) {
        if (posix__pthread_joinable(tidp)) {
            pthread_join(tidp->pid_, retval);
            pthread_attr_destroy(&tidp->attr_);
        }
    }
    tidp->pid_ = 0;
#endif
    return 0;
}

void posix__pthread_mutex_init(posix__pthread_mutex_t *mutex) {
#if _WIN32
    InitializeCriticalSection(&mutex->handle_);
#else
    pthread_mutexattr_init(&mutex->attr_);
    /* 为和 WIN32 的临界区保持语义兼容性， MUTEX 默认为递归锁 */
    pthread_mutexattr_settype(&mutex->attr_, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex->handle_, &mutex->attr_);
#endif
}

void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
#if _WIN32
        EnterCriticalSection(&mutex->handle_);
#else
        pthread_mutex_lock(&mutex->handle_);
#endif
    }
}

int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
#if _WIN32
        return convert_boolean_condition_to_retval(TryEnterCriticalSection(&mutex->handle_));
#else
        return pthread_mutex_trylock(&mutex->handle_);
#endif
    }
    return -1;
}

void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex) {
    if (mutex) {
#if _WIN32
        LeaveCriticalSection(&mutex->handle_);
#else
        pthread_mutex_unlock(&mutex->handle_);
#endif
    }
}

void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex) {
    if (mutex) {
#if _WIN32
        DeleteCriticalSection(&mutex->handle_);
#else
        pthread_mutexattr_destroy(&mutex->attr_);
        pthread_mutex_destroy(&mutex->handle_);
#endif
    }
}

void posix__pthread_yield() {
#if _WIN32
    SwitchToThread();
#else
    syscall(__NR_sched_yield);
#endif
}