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

/* ��ȡϵͳʱ��δ� ���뵥λ */
__extern__
uint64_t posix__gettick();
/* ��ȡϵͳʱ��δ� 100���뵥λ */
__extern__
uint64_t posix__clock_gettime();
__extern__
int posix__localtime(posix__systime_t *systime);
/* ��ȡ����ʱ�� 1970-01-01 00:00:00 000 �������������ŵĺ�����  */
__extern__
uint64_t posix__clock_epoch();


#endif /* POSIX_TIME_H */