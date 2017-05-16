/* gcc -ldl */
#ifndef POSIX_IFOS_H
#define POSIX_IFOS_H


#include "compiler.h"

/* ifos-ps */
__extern__
int posix__gettid();
__extern__
int posix__getpid();
/*
 * posix__sleep ������ linux �޷���ȷ�����룬 �����Ҫ��ȷ�����룬 ��ʹ�� waitable_handle �ĳ�ʱ����
 */
__extern__
void posix__sleep(uint64_t ms);

/* ifos-dl */
__extern__
void *posix__dlopen(const char *file);
__extern__
void* posix__dlsym(void* handle, const char* symbol);
__extern__
int posix__dlclose(void *handle);
__extern__
const char *posix__dlerror();

/* ifos-dir/file posix__pmkdir ��������ݹ鹹��Ŀ¼��*/
__extern__
int posix__mkdir(const char *const dir);
__extern__
int posix__pmkdir(const char *const dir);

/* ��� @target ָ��Ŀ¼�� ��Ը�Ŀ¼ִ�еݹ�ɾ�� rm -rf */
__extern__
int posix__rm(const char *const target);
__extern__
void posix__close(int fd);

/* ��ȡ��ǰִ���ļ�����·�� */
__extern__
const char *posix__fullpath_current();

/* ��ȡ��ǰִ���ļ���������Ŀ¼ */
__extern__
const char *posix__getpedir();
__extern__
const char *posix__getpename();
__extern__
const char *posix__getelfname();
__extern__
const char *posix__gettmpdir();
__extern__
int posix__isdir(const char *const file);

/*ifos-ps*/

/* ��ȡ��ǰ�������ȼ�
 * @priority ���ؽ������ȼ��� ����Ϊ��
 *  */
__extern__
int posix__getpriority(int *priority);

/* �����������ȼ�
 * linux �ṩ 5,0,-5,-10 �ĸ����õ��ε����ȼ�
 * win32 ��Ӧ IDLE_PRIORITY_CLASS NORMAL_PRIORITY_CLASS HIGH_PRIORITY_CLASS REALTIME_PRIORITY_CLASS �ĸ����õ��ε����ȼ�
 *  */
__extern__
int posix__setpriority_below();
__extern__
int posix__setpriority_normal();
__extern__
int posix__setpriority_critical();
__extern__
int posix__setpriority_realtime();

/* ��ȡCPU�������� */
__extern__
int posix__getnpros();

/* ��ȡϵͳ�ڴ���Ϣ */
typedef struct {
    uint64_t totalram;
    uint64_t freeram;
    uint64_t totalswap;
    uint64_t freeswap;
} sys_memory_t;

__extern__
int posix__getsysmem(sys_memory_t *sysmem);

/* ��ȡϵͳ��ҳ��С */
__extern__
uint32_t posix__getpagesize();

/* ����ϵͳ����־ */
__extern__
void posix__syslog(const char *const logmsg );

/* �����ʽת�� 
 * from_encode/to_encode ֧���б�:
 * utf-8
 * gb2312
 * unicode
 */
__extern__
int posix__iconv(const char *from_encode, const char *to_encode, char **from, size_t from_bytes, char **to, size_t *to_bytes);

/* ��ͬ����д�ļ��� ��ȷ����д���ȷ��ϵ�������
 * @����ֵ: ������ɶ�д���ֽ��������᷵�ظ��� */
__extern__
int posix__write_file(int fd, const char *buffer, int size);
__extern__
int posix__read_file(int fd, char *buffer, int size);

/*  Generate random numbers in the half-closed interval
 *  [range_min, range_max). In other words,
 *  range_min <= random number < range_max
 */
__extern__
int posix__random(const int range_min, const int range_max);

#endif /* POSIX_IFOS_H */

