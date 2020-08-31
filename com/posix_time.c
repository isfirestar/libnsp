#include "compiler.h"
#include "posix_time.h"

#include <time.h>

/* convert to 100ns */
static const uint64_t ET_METHOD_NTKRNL = ((uint64_t) ((uint64_t) 1000 * 1000 * 10));

#if _WIN32

#include <Windows.h>

/* NT FILETIME 到 Epoch 时间的差距， 单位100ns(NT FILETIME采用1640年记时)
  使用ULL强制限制数据类型， 避免 warning: this decimal constant is unsigned only in ISO C90 警告 */
static const uint64_t NT_EPOCH_ESCAPE = (uint64_t) ((uint64_t) ((uint64_t) 27111902ULL << 32) | 3577643008ULL);
/* { .dwLowDateTime = 3577643008, .dwHighDateTime = 27111902 }; */

int posix__clock_localtime(posix__systime_t *systime)
{
    uint64_t nt_filetime;
    FILETIME file_now, local_file_now;
    SYSTEMTIME sys_now;

    nt_filetime = systime->epoch + NT_EPOCH_ESCAPE;

    file_now.dwLowDateTime = nt_filetime & 0xFFFFFFFF;
    file_now.dwHighDateTime = (nt_filetime >> 32) & 0xFFFFFFFF;
    FileTimeToLocalFileTime(&file_now, &local_file_now);

    FileTimeToSystemTime(&local_file_now, &sys_now);

    systime->year = sys_now.wYear;
    systime->month = sys_now.wMonth;
    systime->day = sys_now.wDay;
    systime->hour = sys_now.wHour;
    systime->minute = sys_now.wMinute;
    systime->second = sys_now.wSecond;
    systime->low = systime->epoch % ET_METHOD_NTKRNL;

    return 0;
}

int posix__localtime_clock(posix__systime_t *systime)
{
    SYSTEMTIME now;
    FILETIME fnow;
    uint64_t nt_file_time;

    now.wYear = systime->year;
    now.wMonth = systime->month;
    now.wDay = systime->day;
    now.wHour = systime->hour;
    now.wMinute = systime->minute;
    now.wSecond = systime->second;
    now.wMilliseconds = 0; /* systime->low / 10000; */

    SystemTimeToFileTime(&now, &fnow);

    nt_file_time = (uint64_t) ((uint64_t) fnow.dwHighDateTime << 32) | fnow.dwLowDateTime;
    nt_file_time += systime->low;
    systime->epoch = nt_file_time - NT_EPOCH_ESCAPE;
    return 0;
}

uint64_t posix__gettick() {
#if _WIN32_WINNT > _WIN32_WINNT_VISTA
    return GetTickCount64();
#else
    return GetTickCount();
#endif
}

uint64_t posix__clock_epoch()
{
    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t epoch;

    GetSystemTime(&system_time);
    if (SystemTimeToFileTime(&system_time, &file_time)) {
        epoch = (uint64_t) ((uint64_t) file_time.dwHighDateTime << 32 | file_time.dwLowDateTime);
        epoch -= NT_EPOCH_ESCAPE;
        return epoch;
    }
    return 0;
}

uint64_t posix__clock_gettime()
{
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
}

uint64_t posix__clock_monotonic()
{
    return posix__clock_gettime();
}

#else /* POSIX */

#include <sys/time.h>

int posix__clock_localtime(posix__systime_t *systime)
{
    struct timeval tv_now;
    struct tm tm_now, *tm_retval;

    if(!systime) {
        return -EINVAL;
    }

    tv_now.tv_sec = systime->epoch / ET_METHOD_NTKRNL;/* 10000000*/

    tm_retval = localtime_r(&tv_now.tv_sec, &tm_now);
    if (!tm_retval) {
        return -1;
    }

    systime->year = tm_now.tm_year + 1900;
    systime->month = tm_now.tm_mon + 1;
    systime->day = tm_now.tm_mday;
    systime->hour = tm_now.tm_hour;
    systime->minute = tm_now.tm_min;
    systime->second = tm_now.tm_sec;
    systime->low = systime->epoch % ET_METHOD_NTKRNL;
    return 0;
}

int posix__localtime_clock(posix__systime_t *systime)
{
    struct tm timem;
    uint64_t epoch;

    if(!systime) {
        return -EINVAL;
    }

    timem.tm_year = systime->year - 1900;
    timem.tm_mon = systime->month - 1;
    timem.tm_mday = systime->day;
    timem.tm_hour = systime->hour;
    timem.tm_min = systime->minute;
    timem.tm_sec = systime->second;

    epoch = (uint64_t) mktime(&timem);
    if ((time_t) epoch == (time_t) - 1) {
        return -1;
    }

    systime->epoch = epoch; /* second */
    systime->epoch *= 10000000; /* 100ns */
    systime->epoch += systime->low; /* ms->100ns */
    return 0;
}

uint64_t posix__gettick()
{
    struct timespec ts;

    if (0 == clock_gettime(CLOCK_MONOTONIC, &ts)) {
        return ((uint64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
    return 0;
}

uint64_t posix__clock_epoch()
{
    struct timespec ts;

    if (0 == clock_gettime(CLOCK_REALTIME, &ts)) {
        /* force format to 10000000 aligned */
        return ((uint64_t) ts.tv_sec * ET_METHOD_NTKRNL + ts.tv_nsec / 100);
    }
    return 0;
}

uint64_t posix__clock_gettime()
{
    /* gcc -lrt */
    struct timespec tsc;
    uint64_t tick;

    /* CLOCK_REALTIME */
    if (0 == clock_gettime(CLOCK_MONOTONIC, &tsc)) {
        /* force format to 10000000 aligned */
        tick = (uint64_t) tsc.tv_sec * ET_METHOD_NTKRNL + tsc.tv_nsec / 100;
        return  tick;
    }

    return 0;
}

uint64_t posix__clock_monotonic()
{
    /* gcc -lrt */
    struct timespec tsc;
    uint64_t tick;

    /* CLOCK_REALTIME */
    if (0 == clock_gettime(CLOCK_MONOTONIC_RAW, &tsc)) {
        /* force format to 10000000 aligned */
        tick = (uint64_t) tsc.tv_sec * ET_METHOD_NTKRNL + tsc.tv_nsec / 100;
        return  tick;
    }

    return 0;
}

#endif


int posix__localtime(posix__systime_t *systime)
{
    if (!systime) {
        return -EINVAL;
    }

    systime->epoch = posix__clock_epoch();
    if (0 != systime->epoch) {
        return posix__clock_localtime(systime);
    }

    return -1;
}
