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
    uint64_t clock_now;
} posix__systime_t;

#pragma pack(pop)

/* ��ȡϵͳʱ��δ� ���뵥λ */
__extern__
uint64_t posix__gettick();

/* ��ȡϵͳʱ��δ� 100���뵥λ */
__extern__
uint64_t posix__clock_gettime();

/* ��ȡ����ʱ�� 1970-01-01 00:00:00 000 �������������ŵĺ�����  */
__extern__
uint64_t posix__clock_epoch();

/* ��ȡ��ǰʱ�� */
__extern__
int posix__localtime(posix__systime_t *systime);
/* ���� clock_now ��ȡ������ʱ���� */
__extern__
int posix__clock_localtime(posix__systime_t *systime);
/* ���� systime �е�������ʱ���룬 ���� clock_now */
__extern__
int posix__localtime_clock(posix__systime_t *systime);



#endif /* POSIX_TIME_H */