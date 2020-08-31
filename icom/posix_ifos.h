/* gcc -ldl */
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
__interface__
long posix__gettid();
__interface__
long posix__getpid();

/* ifos-getspnam */
__interface__
int posix__syslogin(const char *user, const char *key);

__interface__
void posix__sleep(uint64_t ms);

/* ifos-dl */
__interface__
void *posix__dlopen(const char *file);
__interface__
void* posix__dlsym(void* handle, const char* symbol);
__interface__
int posix__dlclose(void *handle);
__interface__
const char *posix__dlerror();
__interface__
const char *posix__dlerror2(char *estr);

/* @posix__pmkdir Allow recursive construction of directory trees */
__interface__
int posix__mkdir(const char *const dir);
__interface__
int posix__pmkdir(const char *const dir);

/* if @target is a directory, this method is the same as rm -fr */
__interface__
int posix__rm(const char *const target);

/* obtain the fully path of current execute file(ELF/PE) */
__interface__
const char *posix__fullpath_current();
__interface__
char *posix__fullpath_current2(char *holder, int cb);	/* thread safe method, version > 9.6.0 */

/* obtain the directory contain current execute file(ELF/PE) */
__interface__
const char *posix__getpedir();
__interface__
char *posix__getpedir2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__interface__
const char *posix__getpename();
__interface__
char *posix__getpename2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__interface__
const char *posix__getelfname();
__interface__
char *posix__getelfname2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__interface__
const char *posix__gettmpdir();
__interface__
char *posix__gettmpdir2(char *holder, int cb); /* thread safe method, version > 9.6.0 */
__interface__
int posix__isdir(const char *const file); /* inner syscall failed, function return -1, not a dir return 0, is dir, return 0x4000 on linux 0x10 on win32 */

/*ifos-ps*/

/* obtain or adjust the priority of process
 * support 5,0,-5,-10 priority level on Linux,
 * corresponding to IDLE_PRIORITY_CLASS NORMAL_PRIORITY_CLASS HIGH_PRIORITY_CLASS REALTIME_PRIORITY_CLASS on MS-API
 */
__interface__
int posix__getpriority(int *priority);
__interface__
int posix__setpriority_below();
__interface__
int posix__setpriority_normal();
__interface__
int posix__setpriority_critical();
__interface__
int posix__setpriority_realtime();

/* obtain or adjust the affinity of process and CPU core.
 * notes that : MS-API use bit mask to describe the affinity attribute,but Linux without it.
 *	for portable reason, using bit-mask here unified
 */
__interface__
int posix__setaffinity_process(int mask);
__interface__
int posix__getaffinity_process(int *mask);

/* obtain the CPU core-count in this machine */
__interface__
int posix__getnprocs();

/* obtain the system meory info */
typedef struct {
    uint64_t totalram;
    uint64_t freeram;
    uint64_t totalswap;
    uint64_t freeswap;
} sys_memory_t;

__interface__
int posix__getsysmem(sys_memory_t *sysmem);

/* get the system memory page size */
__interface__
uint32_t posix__getpagesize();

/* wirte syslog */
__interface__
void posix__syslog(const char *const logmsg );

/* cover text encoder, support list:
 * UTF-8
 * GB2312
 * UNICODE
 */
__interface__
int posix__iconv(const char *from_encode, const char *to_encode, char **from, size_t from_bytes, char **to, size_t *to_bytes);

/*  Generate random numbers in the half-closed interval
 *  [range_min, range_max). In other words,
 *  range_min <= random number < range_max
 */
__interface__
int posix__random(const int range_min, const int range_max);
__interface__
int posix__random_block(unsigned char *buffer, int size);

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

/* newest file framework version > 9.8.0 */
/* simple file operations */
/* lowest 1 bit to describe open access mode, 0 means read only */
#define FF_RDACCESS			(0)
#define FF_WRACCESS			(1)
/* next 3 bit to describe open method */
#define FF_OPEN_EXISTING	(2)		/* failed on file NOT existed */
#define FF_OPEN_ALWAYS		(4)		/* create a new file when file NOT existed, otherwise open existing */
#define FF_CREATE_NEWONE	(6)		/* failed on file existed  */
#define FF_CREATE_ALWAYS	(8)		/* truncate and open file when it is existed, otherwise create new one with zero size */
/* windows application ignore @mode parameter
   @descriptor return the file-descriptor/file-handle when all syscall successed */
__interface__
int posix__file_open(const char *path, int flag, int mode, file_descriptor_t *descriptor);
__interface__
int64_t posix__file_fgetsize(file_descriptor_t fd);
__interface__
int64_t posix__file_getsize(const char *path);
__interface__
int posix__file_seek(file_descriptor_t fd, uint64_t offset);
__interface__
int posix__file_read(file_descriptor_t fd, void *buffer, int size);
__interface__
int posix__file_write(file_descriptor_t fd, const void *buffer, int size);
__interface__
void posix__file_close(file_descriptor_t fd);
__interface__
int posix__file_flush(file_descriptor_t fd);

#if !defined EBADFD
#define EBADFD	77
#endif

#endif /* POSIX_IFOS_H */
