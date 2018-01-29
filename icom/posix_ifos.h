/* gcc -ldl */
#ifndef POSIX_IFOS_H
#define POSIX_IFOS_H


#include "compiler.h"

/* ifos-ps */
__extern__
long posix__gettid();
__extern__
long posix__getpid();
/*
 * posix__sleep 过程在 linux 无法精确到毫秒， 如果需要精确到毫秒， 则使用 waitable_handle 的超时机制
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

/* ifos-dir/file posix__pmkdir 方法允许递归构建目录树*/
__extern__
int posix__mkdir(const char *const dir);
__extern__
int posix__pmkdir(const char *const dir);

/* 如果 @target 指定目录， 则对该目录执行递归删除 rm -rf */
__extern__
int posix__rm(const char *const target);
__extern__
void posix__close(int fd);
__extern__
int posix__fflush(int fd);

/* 获取当前执行文件完整路径 */
__extern__
const char *posix__fullpath_current();
__extern__
char *posix__fullpath_current2(char *holder, int cb);	/* thread safe method, version > 9.6.0 */

/* 获取当前执行文件及其所在目录 */
__extern__
const char *posix__getpedir();
__extern__
char *posix__getpedir2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__extern__
const char *posix__getpename();
__extern__
char *posix__getpename2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__extern__
const char *posix__getelfname();
__extern__
char *posix__getelfname2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__extern__
const char *posix__gettmpdir();
__extern__
char *posix__gettmpdir2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__extern__
int posix__isdir(const char *const file);

/*ifos-ps*/

/* 获取当前进程优先级
 * @priority 返回进程优先级， 不能为空
 *  */
__extern__
int posix__getpriority(int *priority);

/* 调整进程优先级
 * linux 提供 5,0,-5,-10 四个内置档次的优先级
 * win32 对应 IDLE_PRIORITY_CLASS NORMAL_PRIORITY_CLASS HIGH_PRIORITY_CLASS REALTIME_PRIORITY_CLASS 四个内置档次的优先级
 *  */
__extern__
int posix__setpriority_below();
__extern__
int posix__setpriority_normal();
__extern__
int posix__setpriority_critical();
__extern__
int posix__setpriority_realtime();

/* 获取CPU核心数量 */
__extern__
int posix__getnprocs();

/* 获取系统内存信息 */
typedef struct {
    uint64_t totalram;
    uint64_t freeram;
    uint64_t totalswap;
    uint64_t freeswap;
} sys_memory_t;

__extern__
int posix__getsysmem(sys_memory_t *sysmem);

/* 获取系统分页大小 */
__extern__
uint32_t posix__getpagesize();

/* 计入系统级日志 */
__extern__
void posix__syslog(const char *const logmsg );

/* 编码格式转换 
 * from_encode/to_encode 支持列表:
 * utf-8
 * gb2312
 * unicode
 */
__extern__
int posix__iconv(const char *from_encode, const char *to_encode, char **from, size_t from_bytes, char **to, size_t *to_bytes);

/* 简单同步读写文件， 并确保读写长度符合调用需求
 * @返回值: 最终完成读写的字节数，不会返回负数 */
__extern__
int posix__write_file(int fd, const char *buffer, int size);
__extern__
int posix__read_file(int fd, char *buffer, int size);
__extern__
uint64_t posix__get_filesize(const char *path);
__extern__
int posix__seek_file_offset(int fd, uint64_t offset);

/*  Generate random numbers in the half-closed interval
 *  [range_min, range_max). In other words,
 *  range_min <= random number < range_max
 */
__extern__
int posix__random(const int range_min, const int range_max);

/* CREAT/OPEN file with normal priority access, file descriptor output by @*descriptor when function return 0 */
/* open existing */
__extern__
int posix__file_open(const char *path, void *descriptor);
/* open always */
__extern__
int posix__file_open_always(const char *path, void *descriptor);
/* create new */
__extern__
int posix__file_create(const char *path, void *descriptor);
/* create always */
__extern__
int posix__file_create_always(const char *path, void *descriptor);



#endif /* POSIX_IFOS_H */

