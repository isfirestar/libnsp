#ifndef POSIX_TIME_H
#define POSIX_TIME_H

#include "compiler.h"

#pragma pack (push, 1)

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int milli_second;
} posix__systime_t;

#pragma pack(pop)

/* 获取系统时间滴答， 毫秒单位 */
__extern__
uint64_t posix__gettick();
/* 获取系统时间滴答， 100纳秒单位 */
__extern__
uint64_t posix__clock_gettime();
__extern__
int posix__localtime(posix__systime_t *systime);
/* 获取绝对时间 1970-01-01 00:00:00 000 到函数调用流逝的毫秒数  */
__extern__
uint64_t posix__clock_epoch();


#endif /* POSIX_TIME_H */