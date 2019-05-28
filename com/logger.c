#include <stdlib.h>
#include <stdio.h>

#if _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdarg.h>
#include <assert.h>

#include "logger.h"
#include "clist.h"

#include "compiler.h"
#include "posix_string.h"
#include "posix_thread.h"
#include "posix_time.h"
#include "posix_wait.h"
#include "posix_ifos.h"
#include "posix_atomic.h"

/* the upper limit of the number of rows in a log file  */
#define  MAXIMUM_LOGFILE_LINE    (5000)

/* maximum asynchronous cache */
#define MAXIMUM_LOGSAVE_COUNT       (3000)

/* maximum number of concurrent threads for log_safe */
#define MAXIMUM_NUMBEROF_CONCURRENT_THREADS   (2000)

static const char *LOG__LEVEL_TXT[] = {
    "info", "warning", "error", "fatal", "trace"
};

typedef struct {
    struct list_head link_;
#if _WIN32
    HANDLE fd_;
#else
    int fd_;
#endif
    posix__systime_t filest_;
    int line_count_;
    char module_[LOG_MODULE_NAME_LEN];
} log__file_describe_t;

static LIST_HEAD(__log__file_head); /* list<log__file_describe_t> */
static posix__pthread_mutex_t __log_file_lock;
static char __log_root_directory[MAXPATH] = { 0 };

typedef struct {
    struct list_head link_;
    char logstr_[MAXIMUM_LOG_BUFFER_SIZE];
    int target_;
    posix__systime_t logst_;
    int tid_;
    enum log__levels level_;
    char module_[LOG_MODULE_NAME_LEN];
} log__async_node_t;

typedef struct {
    int pendding_;
    struct list_head items_; /* list<log__async_node_t> */
    posix__pthread_mutex_t lock_;
    posix__pthread_t thread_;
    posix__waitable_handle_t alert_;
    log__async_node_t *misc_memory;
    uint64_t misc_index;    /* it must be unsigned int, if not, increase it may be get a negative number and cause the addressing of @misc_memory crash */
} log__async_context_t;

static log__async_context_t __log_async;

static
int log__create_file(log__file_describe_t *file, const char *path) {
    if (!file || !path) {
        return -1;
    }

#if _WIN32
    file->fd_ = CreateFileA(path, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == file->fd_) {
        return -1;
    }
    SetFilePointer(file->fd_, 0, NULL, FILE_END);
    return 0;
#else
    file->fd_ = open(path, O_RDWR | O_APPEND);
    if (file->fd_ < 0) {
        if (ENOENT == errno) {
            file->fd_ = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
    }
    return file->fd_;
#endif
}

static
void log__close_file(log__file_describe_t *file) {
    if (file) {

#if _WIN32
        if (INVALID_HANDLE_VALUE != file->fd_) {
            CloseHandle(file->fd_);
            file->fd_ = INVALID_HANDLE_VALUE;
#else
        if (file->fd_ >= 0) {
            close(file->fd_);
            file->fd_ = -1;
#endif
        }
    }
}

static
int log__fwrite(log__file_describe_t *file, const void *buf, int count) {
#if _WIN32
    uint32_t out;
    if (!WriteFile(file->fd_, buf, count, &out, NULL)) {
        return -1;
    }
#else
    int offset, written;
    const unsigned char *p;

    offset = 0;
    p = (const unsigned char *)buf;
    while (offset < count) {
        written = write(file->fd_, p + offset, count - offset);
        if (written < 0) {
            if (errno != EINTR) {
                return -1;
            }
        } else {
            if (0 == written) {
                break;
            }
            offset += written;
        }
    }

#endif

    /* posix__fflush(file->fd_) */
    file->line_count_++;
    return 0;
}

static
log__file_describe_t *log__attach(const posix__systime_t *currst, const char *module) {
    char name[128], path[512], pename[128];
    int retval;
    struct list_head *pos;
    log__file_describe_t *file;

    if (!currst || !module) {
        return NULL;
    }

    file = NULL;

    list_for_each(pos, &__log__file_head) {
        file = containing_record(pos, log__file_describe_t, link_);
        if (0 == posix__strcasecmp(module, file->module_)) {
            break;
        }
        file = NULL;
    }

    do {
        /* 对象为空，说明该module模块新增 */
        if (!file) {
            if (NULL == (file = malloc(sizeof ( log__file_describe_t)))) {
                return NULL;
            }
            memset(file, 0, sizeof ( log__file_describe_t));
#if _WIN32
            file->fd_ = INVALID_HANDLE_VALUE;
#else
            file->fd_ = -1;
#endif
            list_add_tail(&file->link_, &__log__file_head);
            break;
        }

        if ((int) file->fd_ < 0) {
            break;
        }

        /* If no date switch occurs and the file content record does not exceed the limit number of rows,
            the file is reused directly.  */
        if (file->filest_.day == currst->day &&
                file->filest_.month == currst->month &&
                file->filest_.day == currst->day &&
                file->line_count_ < MAXIMUM_LOGFILE_LINE) {
            return file;
        }

        /* Switching the log file does not reclaim the log object pointer, just closing the file descriptor  */
        log__close_file(file);
    } while (0);

    posix__getpename2(pename, cchof(pename));

    /* New or any form of file switching occurs in log posts  */
    posix__sprintf(name, cchof(name), "%s_%04u%02u%02u_%02u%02u%02u.log", module,
            currst->year, currst->month, currst->day, currst->hour, currst->minute, currst->second);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s"POSIX__DIR_SYMBOL_STR, __log_root_directory, pename );
    posix__pmkdir(path);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s"POSIX__DIR_SYMBOL_STR"%s", __log_root_directory, pename, name);
    retval = log__create_file(file, path);
    if (retval >= 0) {
        memcpy(&file->filest_, currst, sizeof ( posix__systime_t));
        posix__strcpy(file->module_, cchof(file->module_), module);
        file->line_count_ = 0;
    } else {
        /* If file creation fails, the linked list node needs to be removed  */
        list_del_init(&file->link_);
        free(file);
        file = NULL;
    }

    return file;
}

static
int log__writestd(enum log__levels level, const char* logstr, int cb) {
    int n;
#if _WIN32
    HANDLE std;
    DWORD written;

    if (level == kLogLevel_Error) {
        std = GetStdHandle(STD_ERROR_HANDLE);
    }else{
        std = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if (INVALID_HANDLE_VALUE != std) {
        n = WriteFile(std, logstr, cb, &written, NULL);
    }
#else
    if (level == kLogLevel_Error) {
        n = write(STDERR_FILENO, logstr, cb); /* 2 */
    }else{
        n = write(STDOUT_FILENO, logstr, cb); /* 1 */
    }
#endif
    return n;
}

static
void log__printf(const char *module, enum log__levels level, int target, const posix__systime_t *currst, const char* logstr, int cb) {
    log__file_describe_t *fileptr;

    if (target & kLogTarget_Filesystem) {
        posix__pthread_mutex_lock(&__log_file_lock);
        fileptr = log__attach(currst, module);
        posix__pthread_mutex_unlock(&__log_file_lock);
        if (fileptr) {
            if (log__fwrite(fileptr, logstr, cb) < 0) {
                log__close_file(fileptr);
            }
        }
    }

    if (target & kLogTarget_Stdout) {
        log__writestd(level, logstr, cb);
    }

    if (target & kLogTarget_Sysmesg) {
        posix__syslog(logstr);
    }
}

static
char *log__format_string(enum log__levels level, int tid, const char* format, va_list ap, const posix__systime_t *currst, char *logstr, int cch) {
    int pos;
    char *p;

    if (level >= kLogLevel_Maximum || !currst || !format || !logstr) {
        return NULL;
    }

    p = logstr;
    pos = 0;
    pos += posix__sprintf(&p[pos], cch - pos, "%02u:%02u:%02u %04u ", currst->hour, currst->minute, currst->second, (currst->low / 10000));
    pos += posix__sprintf(&p[pos], cch - pos, "%s ", LOG__LEVEL_TXT[level]);
    pos += posix__sprintf(&p[pos], cch - pos, "%04X # ", tid);
    pos += posix__vsprintf(&p[pos], cch - pos, format, ap);
    pos += posix__sprintf(&p[pos], cch - pos, "%s", POSIX__EOL);

    return p;
}

static
void *log__asnyc_proc(void *argv) {
    log__async_node_t *node;

    while (posix__waitfor_waitable_handle(&__log_async.alert_, 10 * 1000) >= 0) {
        do {
            node = NULL;

            posix__pthread_mutex_lock(&__log_async.lock_);
            if (!list_empty(&__log_async.items_)) {
                posix__atomic_dec(&__log_async.pendding_);
                node = list_first_entry(&__log_async.items_, log__async_node_t, link_);
                assert(node);
                list_del_init(&node->link_);
            }
            posix__pthread_mutex_unlock(&__log_async.lock_);

            if (node) {
                log__printf(node->module_, node->level_, node->target_, &node->logst_, node->logstr_, (int) strlen(node->logstr_));
            }
        } while (node);
    }

    posix__syslog("nsplog asynchronous thread has been terminated.");
    return NULL;
}

static
int log__async_init() {
    int misc_memory_size;

    __log_async.pendding_ = 0;
    misc_memory_size = MAXIMUM_LOGSAVE_COUNT * sizeof(log__async_node_t);

    __log_async.misc_memory = (log__async_node_t *)malloc(misc_memory_size);
    if (!__log_async.misc_memory) {
        return RE_ERROR(ENOMEM);
    }
    memset(__log_async.misc_memory, 0, misc_memory_size);

    __log_async.misc_index = 0;

    INIT_LIST_HEAD(&__log_async.items_);
    posix__pthread_mutex_init(&__log_async.lock_);
    posix__init_synchronous_waitable_handle(&__log_async.alert_);

    if (posix__pthread_create(&__log_async.thread_, &log__asnyc_proc, NULL) < 0) {
        posix__pthread_mutex_release(&__log_async.lock_);
        posix__uninit_waitable_handle(&__log_async.alert_);
        free(__log_async.misc_memory);
        __log_async.misc_memory = NULL;
        return -1;
    }

    return 0;
}

static
void log__create_log_directory(const char *rootdir)
{
    int pos, i;

    if (rootdir) {
#if _WIN32
		strcpy_s(__log_root_directory, cchof(__log_root_directory), rootdir);
#else
        strcpy(__log_root_directory, rootdir);
#endif

        pos = strlen(__log_root_directory);
        for (i = pos - 1; i >= 0; i--) {
            if (__log_root_directory[i] == POSIX__DIR_SYMBOL) {
                __log_root_directory[i] = 0;
            } else {
                break;
            }
        }

		if (posix__pmkdir(__log_root_directory) < 0) {
			posix__getpedir2(__log_root_directory, sizeof(__log_root_directory));
		}
	} else {
		posix__getpedir2(__log_root_directory, sizeof(__log_root_directory));
	}
}

static int __log__init() {
    posix__atomic_initial_declare_variable(__inited__);

    if (posix__atomic_initial_try(&__inited__)) {
       /* initial global context */
        posix__pthread_mutex_init(&__log_file_lock);

        /* allocate the async node pool */
        if ( log__async_init() < 0 ) {
            posix__pthread_mutex_release(&__log_file_lock);
            posix__atomic_initial_exception(&__inited__);
        } else {
            posix__atomic_initial_complete(&__inited__);
        }
    }

    return __inited__;
}

int log__init()
{
	return log__init2(NULL);
}

int log__init2(const char *rootdir)
{
    log__create_log_directory(rootdir);
	return __log__init();
}

void log__write(const char *module, enum log__levels level, int target, const char *format, ...) {
    va_list ap;
    char logstr[MAXIMUM_LOG_BUFFER_SIZE];
    posix__systime_t currst;
    char pename[MAXPATH], *p;

    if (log__init() < 0 || !format || level >= kLogLevel_Maximum || level < 0) {
        return;
    }

    posix__localtime(&currst);

    if (0 == __log_root_directory[0]) {
        posix__getpedir2(__log_root_directory, sizeof(__log_root_directory));
    }

    va_start(ap, format);
    log__format_string(level, posix__gettid(), format, ap, &currst, logstr, cchof(logstr));
    va_end(ap);

    if (!module) {
        p = posix__getpename2(pename, sizeof(pename));
        if (p) {
            log__printf(pename, level, target, &currst, logstr, (int) strlen(logstr));
        }
    } else {
        log__printf(module, level, target, &currst, logstr, (int) strlen(logstr));
    }
}

void log__save(const char *module, enum log__levels level, int target, const char *format, ...) {
    va_list ap;
    log__async_node_t *node;
    uint64_t index;
    char pename[MAXPATH], *p;

    if (log__init() < 0 || !format || level >= kLogLevel_Maximum || level < 0) {
        return;
    }

    /* thread safe in count less or equal to 50 */
    if (posix__atomic_inc(&__log_async.pendding_) >= (MAXIMUM_LOGSAVE_COUNT - MAXIMUM_NUMBEROF_CONCURRENT_THREADS)) {
        posix__atomic_dec(&__log_async.pendding_);
        return;
    }

    /* atomic increase index */
    index = posix__atomic_inc64(&__log_async.misc_index);
    /* hash node at index of memory block */
    node = (log__async_node_t *)&__log_async.misc_memory[index % MAXIMUM_LOGSAVE_COUNT];
    node->target_ = target;
    node->level_ = level;
    posix__localtime(&node->logst_);

    if (0 == __log_root_directory[0]) {
        posix__getpedir2(__log_root_directory, sizeof(__log_root_directory));
    }

    if (module) {
        posix__strcpy(node->module_, cchof(node->module_), module);
    } else {
        p = posix__getpename2(pename, sizeof(pename));
        if (p) {
            posix__strcpy(node->module_, cchof(node->module_), pename);
        }
    }

    va_start(ap, format);
    log__format_string(node->level_, posix__gettid(), format, ap, &node->logst_, node->logstr_, sizeof (node->logstr_));
    va_end(ap);

    posix__pthread_mutex_lock(&__log_async.lock_);
    list_add_tail(&node->link_, &__log_async.items_);
    posix__pthread_mutex_unlock(&__log_async.lock_);
    posix__sig_waitable_handle(&__log_async.alert_);
}
