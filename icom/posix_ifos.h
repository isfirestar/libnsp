﻿/* gcc -ldl */
#ifndef POSIX_IFOS_H
#define POSIX_IFOS_H


#include "compiler.h"

#if _WIN32
	#include <Windows.h>
#else
	#include <unistd.h>
	#include <fcntl.h>
#endif

/* ifos-ps */
__extern__
long posix__gettid();
__extern__
long posix__getpid();

/* ifos-getspnam */
__extern__
int posix__syslogin(const char *user, const char *key);

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
__extern__
const char *posix__dlerror2(char *estr);

/* @posix__pmkdir Allow recursive construction of directory trees */
__extern__
int posix__mkdir(const char *const dir);
__extern__
int posix__pmkdir(const char *const dir);

/* if @target is a directory, this method is the same as rm -fr */
__extern__
int posix__rm(const char *const target);

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

/* 调整进程亲和性
 * linux 系统调用不使用位或，windows系统调用使用位或
 * 为了统一平台接口， 这里一律使用位或
 */
__extern__
int posix__setaffinity_process(int mask);
__extern__
int posix__getaffinity_process(int *mask);

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

/*  Generate random numbers in the half-closed interval
 *  [range_min, range_max). In other words,
 *  range_min <= random number < range_max
 */
__extern__
int posix__random(const int range_min, const int range_max);

#if _WIN32
	typedef HANDLE file_descriptor_t;
	#define INVALID_FILE_DESCRIPTOR		((file_descriptor_t)INVALID_HANDLE_VALUE)

	#if !defined STDIN_FILENO
		#define STDIN_FILENO		(file_descriptor_t)GetStdHandle(STD_INPUT_HANDLE)
	#endif

	#if !defined STDOUT_FILENO
		#define STDOUT_FILENO		(file_descriptor_t)GetStdHandle(STD_OUTPUT_HANDLE)
	#endif

	#if !defined STDERR_FILENO
		#define STDERR_FILENO		(file_descriptor_t)GetStdHandle(STD_ERROR_HANDLE)
	#endif
#else
	typedef int file_descriptor_t;
	#define INVALID_FILE_DESCRIPTOR		((file_descriptor_t)-1)
#endif

/* simple file operations */
/* lowest 1 bit to describe open access mode, 0 means read only */
#define FF_RDACCESS			(0)
#define FF_WRACCESS			(1)
/* next 3 bit to describe open method */
#define FF_OPEN_EXISTING	(2)
#define FF_OPEN_ALWAYS		(4)
#define FF_CREATE_NEWONE	(6)
#define FF_CREATE_ALWAYS	(8)
#define FF_TRUNCTE_ALWAYS	(10)
/* windows application ignore @mode parameter
   @descriptor return the file-descriptor/file-handle when all syscall successed */
__extern__
int posix__file_open(const char *path, int flag, int mode, file_descriptor_t *descriptor);
__extern__
uint64_t posix__file_fgetsize(file_descriptor_t fd);
__extern__
uint64_t posix__file_getsize(const char *path);
__extern__
int posix__file_seek(file_descriptor_t fd, uint64_t offset);
__extern__
int posix__file_read(file_descriptor_t fd, void *buffer, int size);
__extern__
int posix__file_write(file_descriptor_t fd, const void *buffer, int size);
__extern__
void posix__file_close(file_descriptor_t fd);
__extern__
int posix__file_flush(file_descriptor_t fd);

#if !defined EBADFD
#define EBADFD	77
#endif

#endif /* POSIX_IFOS_H */
