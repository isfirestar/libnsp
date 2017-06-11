#include "posix_time.h"

#include <time.h>

#if _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

int posix__localtime(posix__systime_t *systime) {

#if _WIN32
    SYSTEMTIME now;
    GetLocalTime(&now);

    if (!systime) return -1;

    systime->year = now.wYear;
    systime->month = now.wMonth;
    systime->day = now.wDay;
    systime->hour = now.wHour;
    systime->minute = now.wMinute;
    systime->second = now.wSecond;
    systime->milli_second = now.wMilliseconds;
#else
    struct timeval tv_now;
    struct tm tm_now;

    if (!systime) return -1;

    gettimeofday(&tv_now, NULL);
    localtime_r(&tv_now.tv_sec, &tm_now);

    systime->year = tm_now.tm_year + 1900;
    systime->month = tm_now.tm_mon + 1;
    systime->day = tm_now.tm_mday;
    systime->hour = tm_now.tm_hour;
    systime->minute = tm_now.tm_min;
    systime->second = tm_now.tm_sec;
    systime->milli_second = (int) tv_now.tv_usec / 1000;
#endif

    return 0;
}

uint64_t posix__gettick() {
#if _WIN32
#if _WIN32_WINNT > _WIN32_WINNT_VISTA
    return GetTickCount64();
#else
    return GetTickCount();
#endif
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ( ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

uint64_t posix__clock_gettime() {
    static const uint64_t ET_METHOD_NTKRNL = ((uint64_t) ((uint64_t) 1000 * 1000 * 10));
#if _WIN32
    LARGE_INTEGER counter;
    static LARGE_INTEGER frequency = {0};
    if (0 == frequency.QuadPart) {
        if (!QueryPerformanceFrequency(&frequency)) {
            return 0;
        }
    }

    if (QueryPerformanceCounter(&counter)) {
        return (uint64_t) (ET_METHOD_NTKRNL * ((double) counter.QuadPart / frequency.QuadPart));
    }
    return 0;
#else
    /* gcc -lrt */
    struct timespec tsc;
    if (clock_gettime(CLOCK_MONOTONIC, &tsc) >= 0) { /* CLOCK_REALTIME */
        return (uint64_t) tsc.tv_sec * /*10000000*/ET_METHOD_NTKRNL + tsc.tv_nsec / 100; /* 返回 100ns, 兼容windows的KRNL计时 */
    }
    return 0;
#endif
}