#ifndef POSIX_TIME_H
#define POSIX_TIME_H

#include "compiler.h"

struct __posix__systime_t {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    uint64_t low; /* 100ns */
    uint64_t epoch;
} __POSIX_TYPE_ALIGNED__;

typedef struct __posix__systime_t posix__systime_t;

/* obtain the wall clock on current system, in 1 second */
__interface__
uint64_t posix__gettick();

/* obtain the wall clock on current system, in 100ns */
__interface__
uint64_t posix__clock_gettime();
__interface__
uint64_t posix__clock_monotonic();

/* obtain absolute time elapse from 1970-01-01 00:00:00 000 to the time point of function invoked, in 100ns
 */
__interface__
uint64_t posix__clock_epoch();

/* obtain the current wall clock by epoch */
__interface__
int posix__localtime(posix__systime_t *systime);

/* obtain year-month-day-time by @systime.epoch */
__interface__
int posix__clock_localtime(posix__systime_t *systime);
/* obtain epoch by @systim.year-month-day-time */
__interface__
int posix__localtime_clock(posix__systime_t *systime);

#endif /* POSIX_TIME_H */
