#include "posix_time.h"

#include <time.h>

#if _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

// 转换为100ns
static const uint64_t ET_METHOD_NTKRNL = ((uint64_t) ((uint64_t) 1000 * 1000 * 10));
// NT FILETIME 到 Epoch 时间的差距， 单位100ns(NT FILETIME采用1640年记时)
static const uint64_t NT_EPOCH_ESCAPE = (uint64_t) ((uint64_t) ((uint64_t) 27111902 << 32) | 3577643008); //{ .dwLowDateTime = 3577643008, .dwHighDateTime = 27111902 };

int posix__localtime(posix__systime_t *systime) {
    if (!systime) {
        return -EINVAL;
    }

    systime->epoch = posix__clock_epoch();
    return posix__clock_localtime(systime);
    //    gettimeofday(&tv_now, NULL);
    //    localtime_r(&tv_now.tv_sec, &tm_now);
    return 0;
}

int posix__clock_localtime(posix__systime_t *systime) {
#if _WIN32
    uint64_t nt_filetime = systime->epoch + NT_EPOCH_ESCAPE;
    FILETIME file_now, local_now;
    file_now.dwLowDateTime = nt_filetime & 0xFFFFFFFF;
    file_now.dwHighDateTime = (nt_filetime >> 32) & 0xFFFFFFFF;
    FileTimeToLocalFileTime(&file_now, &local_now);

    SYSTEMTIME sys_now;
    FileTimeToSystemTime(&local_now, &sys_now);

    systime->year = sys_now.wYear;
    systime->month = sys_now.wMonth;
    systime->day = sys_now.wDay;
    systime->hour = sys_now.wHour;
    systime->minute = sys_now.wMinute;
    systime->second = sys_now.wSecond;
    systime->low = systime->epoch % ET_METHOD_NTKRNL;
#else
    struct timeval tv_now;
    struct tm tm_now;

    systime->epoch = posix__clock_epoch();

    tv_now.tv_sec = systime->epoch / /* 10000000*/ET_METHOD_NTKRNL;

    localtime_r(&tv_now.tv_sec, &tm_now);

    systime->year = tm_now.tm_year + 1900;
    systime->month = tm_now.tm_mon + 1;
    systime->day = tm_now.tm_mday;
    systime->hour = tm_now.tm_hour;
    systime->minute = tm_now.tm_min;
    systime->second = tm_now.tm_sec;
    systime->low = systime->epoch % ET_METHOD_NTKRNL;
#endif
    return 0;
}

int posix__localtime_clock(posix__systime_t *systime) {
#if _WIN32
    SYSTEMTIME now;
    FILETIME fnow;

    now.wYear = systime->year;
    now.wMonth = systime->month;
    now.wDay = systime->day;
    now.wHour = systime->hour;
    now.wMinute = systime->minute;
    now.wSecond = systime->second;
    now.wMilliseconds = 0; // systime->low / 10000;

    SystemTimeToFileTime(&now, &fnow);

    uint64_t nt_filetime = (uint64_t) ((uint64_t) fnow.dwHighDateTime << 32) | fnow.dwLowDateTime;
    nt_filetime += systime->low;
    systime->epoch = nt_filetime - NT_EPOCH_ESCAPE;
#else
    struct tm timem;
    uint64_t epoch;

    if (!systime) return -1;

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

    systime->epoch = epoch; // 秒
    systime->epoch *= 10000000; // 100ns
    systime->epoch += systime->low; // ms->100ns
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
    return ( (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

uint64_t posix__clock_epoch() {
#if _WIN32
    SYSTEMTIME local_time;
    FILETIME file_time;
    GetLocalTime(&local_time);
    if (SystemTimeToFileTime(&local_time, &file_time)) {
        uint64_t epoch = (uint64_t) ((uint64_t) file_time.dwHighDateTime << 32 | file_time.dwLowDateTime);
        epoch -= NT_EPOCH_ESCAPE;
        return epoch;
    }
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return ((uint64_t) ts.tv_sec * /*10000000*/ET_METHOD_NTKRNL + ts.tv_nsec / 100);
    }
#endif
    return 0;
}

uint64_t posix__clock_gettime() {
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