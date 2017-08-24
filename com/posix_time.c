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
    FILETIME fnow;
    
    GetLocalTime(&now);

    if (!systime) return -1;

    systime->year = now.wYear;
    systime->month = now.wMonth;
    systime->day = now.wDay;
    systime->hour = now.wHour;
    systime->minute = now.wMinute;
    systime->second = now.wSecond;
    systime->milli_second = now.wMilliseconds;
    
    SystemTimeToFileTime(&now, &fnow);
    
    systime->clock_now = fnow.dwHighDateTime;
    systime->clock_now <<= 32;
    systime->clock_now |= fnow.dwLowDateTime;
#else
    struct timeval tv_now;
    struct tm tm_now;

    if (!systime) return -1;

    gettimeofday(&tv_now, NULL);
    
    systime->clock_now = tv_now.tv_sec;
    systime->clock_now *= 1000000;
    systime->clock_now += tv_now.tv_usec;
    
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

int posix__clock_localtime(posix__systime_t *systime) {

#if _WIN32
    SYSTEMTIME now;
    FILETIME fnow;
    
    GetLocalTime(&now);

    if (!systime) return -1;
    if (systime->clock_now == 0) return -1;
    
    fnow.dwLowDateTime = (systime->clock_now & 0xFFFFFFFF);
    fnow.dwHighDateTime = (() systime->clock_now >> 32 ) & 0xFFFFFFFF);
    
    FileTimeToSystemTime(&fnow, &now);
    
    systime->year = now.wYear;
    systime->month = now.wMonth;
    systime->day = now.wDay;
    systime->hour = now.wHour;
    systime->minute = now.wMinute;
    systime->second = now.wSecond;
    systime->milli_second = now.wMilliseconds;
    
    SystemTimeToFileTime(&now, &fnow);
    
    systime->clock_now = fnow.dwHighDateTime;
    systime->clock_now <<= 32;
    systime->clock_now |= fnow.dwLowDateTime;
#else
    struct timeval tv_now;
    struct tm tm_now;

    if (!systime) return -1;
    if (systime->clock_now == 0) return -1;

    tv_now.tv_sec = systime->clock_now / 1000000;
    tv_now.tv_usec = systime->clock_now % 1000000;

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
    now.wMilliseconds = systime->milli_second;
    
    
#else
    struct tm timem;
    time_t epoch;
    
    if (!systime) return -1;
    
    timem.tm_year = systime->year - 1900;
    timem.tm_mon = systime->month - 1;
    timem.tm_mday = systime->day;
    timem.tm_hour = systime->hour;
    timem.tm_min = systime->minute;
    timem.tm_sec = systime->second;
    
    epoch = mktime(&timem);
    if (epoch == (time_t)-1) {
        return -1;
    }
    
    systime->clock_now = epoch; // 秒
    systime->clock_now *= 1000000;
    systime->clock_now += (systime->milli_second * 1000); // 精确到微妙
    
    return 0;
#endif
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
        return (uint64_t) ((uint64_t) file_time.dwHighDateTime << 32 | file_time.dwLowDateTime);
    }
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return ( (uint64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
#endif
    return 0;
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