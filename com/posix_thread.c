#include "posix_thread.h"

#if _WIN32

struct WIN32_THPAR {
    void*(*start_rtn_)(void*);
    void *arg_;
};

uint32_t WINAPI ThProc(void* parameter)
{
    struct WIN32_THPAR *par = (struct WIN32_THPAR *) parameter;
    void*(*start_rtn)(void*) = par->start_rtn_;
    void *arg = par->arg_;

    free(par);
    if (start_rtn) {
        return (uint32_t)start_rtn(arg);
    }
    return 0;
}

int posix__pthread_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg)
{
    HANDLE th;
	struct WIN32_THPAR *thpar;

	thpar  = malloc(sizeof (struct WIN32_THPAR));
	if (!thpar) {
		return -ENOMEM;
	}

    thpar->arg_ = arg;
    thpar->start_rtn_ = start_rtn;
    th = CreateThread(NULL, 0, ThProc, thpar, 0, NULL);
    if (!th) {
        return posix__makeerror(GetLastError());
    }

    tidp->detached_ = NO;
    tidp->pid_ = th;
    return 0;
}

int posix__pthread_self(posix__pthread_t *tidp)
{
    if (!tidp) {
        return -EINVAL;
    }

    tidp->pid_ = GetCurrentThread();
    return (int)tidp->pid_;
}

int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg)
{
    HANDLE th;
	struct WIN32_THPAR *thpar;

    if (!tidp) {
        return -EINVAL;
    }

	thpar = malloc(sizeof (struct WIN32_THPAR));
	if (!thpar) {
		return -ENOMEM;
	}

    thpar->arg_ = arg;
    thpar->start_rtn_ = start_rtn;
    th = CreateThread(NULL, 0, ThProc, thpar, 0, NULL);
    if (!th) {
        return posix__makeerror(GetLastError());
    }
    SetThreadPriority(th, THREAD_PRIORITY_TIME_CRITICAL);

    tidp->detached_ = NO;
    tidp->pid_ = th;
    return 0;
}

int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg)
{
    HANDLE th;
	struct WIN32_THPAR *thpar;

    if (!tidp) {
        return -EINVAL;
    }

	thpar = malloc(sizeof (struct WIN32_THPAR));
	if (!thpar) {
		return -ENOMEM;
	}

    thpar->arg_ = arg;
    thpar->start_rtn_ = start_rtn;
    th = CreateThread(NULL, 0, ThProc, thpar, 0, NULL);
    if (!th) {
        return posix__makeerror(GetLastError());
    }

    if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        SetThreadPriority(th, 31);
    }

    tidp->detached_ = NO;
    tidp->pid_ = th;
    return 0;
}

int posix__pthread_setaffinity(const posix__pthread_t *tidp, int mask)
{
    if (0 == mask) {
        return -1;
    }

    if (SetThreadAffinityMask(tidp->pid_, (DWORD_PTR)mask)) {
        return 0;
    }

    return posix__makeerror(GetLastError());
}

int posix__pthread_getaffinity(const posix__pthread_t *tidp, int *mask)
{
    return -1;
}

int posix__pthread_detach(posix__pthread_t * tidp) {
    if (!tidp) {
        return -EINVAL;
    }

	if (NO == posix__pthread_joinable(tidp)) {
		return -1;
	}

    if (tidp->pid_) {
        CloseHandle(tidp->pid_);
    }

    tidp->pid_ = NULL;
    tidp->detached_ = YES;
    return 0;
}

int posix__pthread_join(posix__pthread_t *tidp, void **exitCode)
{
	DWORD LocalExitCode;
	if (!tidp) {
		return -EINVAL;
	}

    if (YES == posix__pthread_joinable(tidp)) {
        WaitForSingleObject(tidp->pid_, INFINITE);
		if (exitCode) {
			GetExitCodeThread(tidp->pid_, (LPDWORD)&LocalExitCode);
			*exitCode = (void *)LocalExitCode;
		}
		CloseHandle(tidp->pid_);
		tidp->pid_ = INVALID_HANDLE_VALUE;
		return 0;
    }

	return -1;
}

int posix__pthread_mutex_init(posix__pthread_mutex_t *mutex)
{
    if (mutex) {
        InitializeCriticalSection(&mutex->handle_);
		return 0;
    }
	return -EINVAL;
}

void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex)
{
    if (mutex) {
        EnterCriticalSection(&mutex->handle_);
    }
}

int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex)
{
    if (!mutex) {
		return -EINVAL;
    }

	if (TryEnterCriticalSection(&mutex->handle_)) {
		return 0;
	}

	return posix__makeerror(GetLastError());
}

int posix__pthread_mutex_timedlock(posix__pthread_mutex_t *mutex, uint32_t expires)
{
    return -1;
}

void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex)
{
    if (mutex) {
        LeaveCriticalSection(&mutex->handle_);
    }
}

void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex)
{
    if (mutex) {
        DeleteCriticalSection(&mutex->handle_);
    }
}

void posix__pthread_yield()
{
    SwitchToThread();
}

#else /* POSIX */

#include <unistd.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/types.h>

int posix__pthread_create(posix__pthread_t *tidp, void*(*start_rtn)(void*), void * arg)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(tidp);
    tidp->detached_ = NO;
    pthread_attr_init(&tidp->attr_);
    retval = pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
    if (0 == retval) {
        return 0; /* success */
    } else {
        return posix__makeerror(retval);
    }
}

int posix__pthread_self(posix__pthread_t *tidp)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(tidp);
    tidp->pid_ = pthread_self();
    return tidp->pid_;
}

int posix__pthread_critical_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(tidp);

    pthread_attr_init(&tidp->attr_);
    if (pthread_attr_setschedpolicy(&tidp->attr_, SCHED_RR) < 0) {
        return errno * -1;
    }

    tidp->detached_ = NO;
    retval = pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
    if (0 == retval) {
        return 0;
    }
    return posix__makeerror(retval);
}

int posix__pthread_realtime_create(posix__pthread_t * tidp, void*(*start_rtn)(void*), void * arg)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(tidp);

    pthread_attr_init(&tidp->attr_);

    /* raiseup policy for realtime thread */
    if (0 == nice(-5)) {
        retval = pthread_attr_setschedpolicy(&tidp->attr_, SCHED_FIFO);
        if ( 0 != retval) {
            return posix__makeerror(retval);
        }
    }

    tidp->detached_ = NO;

    retval = pthread_create(&tidp->pid_, &tidp->attr_, start_rtn, arg);
    if (0 == retval) {
        return 0;
    }
    return posix__makeerror(retval);
}

int posix__pthread_setaffinity(const posix__pthread_t *tidp, int mask)
{
    int i;
    cpu_set_t cpus;
    int retval;

    if (0 == mask) {
        return -1;
    }

    CPU_ZERO(&cpus);

    for (i = 0; i < 32; i++) {
        if (mask & (1 << i)) {
            CPU_SET(i, &cpus);
        }
    }

    retval = pthread_setaffinity_np(tidp->pid_, sizeof(cpu_set_t), &cpus);
    if (0 == retval) {
        return 0;
    }
    return posix__makeerror(retval);
}

int posix__pthread_getaffinity(const posix__pthread_t *tidp, int *mask)
{
    int i;
    cpu_set_t cpus;
    int n;
    int retval;

    n = 0;
    CPU_ZERO(&cpus);
    retval = pthread_getaffinity_np(0, sizeof(cpu_set_t), &cpus);
    if ( 0 != retval ) {
        return posix__makeerror(retval);
    }

    for (i = 0; i < 32; i++) {
        if(CPU_ISSET(i, &cpus)) {
            n |= (1 << i);
        }
    }

    if (mask) {
        *mask = n;
    }

    return 0;
}

int posix__pthread_detach(posix__pthread_t * tidp)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(tidp);

    if (NO == posix__pthread_joinable(tidp)) {
        return -1;
    }

    retval = pthread_attr_setdetachstate(&tidp->attr_, PTHREAD_CREATE_DETACHED);
    if ( 0 == retval ) {
        pthread_attr_destroy(&tidp->attr_);
        tidp->detached_ = YES;
        tidp->pid_ = 0;
        return 0;
    }

    return posix__makeerror(retval);
}

int posix__pthread_join(posix__pthread_t *tidp, void **retval)
{
    int fr;
    pthread_t pid;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(tidp);

    if (YES == posix__pthread_joinable(tidp)) {
        pid = tidp->pid_;
        tidp->pid_ = 0;
        tidp->detached_ = YES;

        fr = pthread_join(pid, retval);
        if (0 != fr) {
            return posix__makeerror(fr);
        }

        pthread_attr_destroy(&tidp->attr_);
    }

    return -1;
}

/*------------------------ posix__pthread_mutex_ methods ------------------------*/
int posix__pthread_mutex_init(posix__pthread_mutex_t *mutex)
{
    pthread_mutexattr_t mutexattr;
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(mutex);

    retval = pthread_mutexattr_init(&mutexattr);
    if (0 != retval) {
        return posix__makeerror(retval);
    }

    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    retval = pthread_mutex_init(&mutex->handle_, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    return posix__makeerror(retval);
}

void posix__pthread_mutex_lock(posix__pthread_mutex_t *mutex)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_NR__(mutex);
    pthread_mutex_lock(&mutex->handle_);
}

int posix__pthread_mutex_trylock(posix__pthread_mutex_t *mutex)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(mutex);
    retval = pthread_mutex_trylock(&mutex->handle_);
    if (0 == retval) {
        return 0;
    }
    return posix__makeerror(retval);
}

int posix__pthread_mutex_timedlock(posix__pthread_mutex_t *mutex, uint32_t expires)
{
    int retval;

    __POSIX_EFFICIENT_ALIGNED_PTR_IR__(mutex);

    struct timespec abstime;
    if (0 == clock_gettime(CLOCK_REALTIME, &abstime)) {
        uint64_t nsec = abstime.tv_nsec;
        nsec += ((uint64_t) expires * 1000000); /* convert milliseconds to nanoseconds */
        abstime.tv_sec += (nsec / 1000000000);
        abstime.tv_nsec = (nsec % 1000000000);
        retval = pthread_mutex_timedlock(&mutex->handle_, &abstime);
        if (0 == retval) {
            return 0;
        }
        return posix__makeerror(retval);
    }
    return posix__makeerror(errno);
}

void posix__pthread_mutex_unlock(posix__pthread_mutex_t *mutex)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_NR__(mutex);
    pthread_mutex_unlock(&mutex->handle_);
}

void posix__pthread_mutex_release(posix__pthread_mutex_t *mutex)
{
    __POSIX_EFFICIENT_ALIGNED_PTR_NR__(mutex);
    pthread_mutex_destroy(&mutex->handle_);
}

void posix__pthread_yield() {
    syscall(__NR_sched_yield);
}

#endif

boolean_t posix__pthread_joinable(posix__pthread_t * tidp)
{
    if (!tidp) {
        return NO;
    }

    if (tidp->pid_ <= 0) {
        return NO;
    }

    return !tidp->detached_;
}
