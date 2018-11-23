#include <stdlib.h>
#include <stdio.h>

#if _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdarg.h>

#include "logger.h"
#include "clist.h"

#include "compiler.h"
#include "posix_string.h"
#include "posix_thread.h"
#include "posix_time.h"
#include "posix_wait.h"
#include "posix_ifos.h"
#include "posix_atomic.h"


#define  MAXIMUM_LOGFILE_LINE    (5000)

/* 最大异步日志存储数量， 达到次数量， 将无法进行异步日志存储 
 * 按每个节点 2KB + 180 字节算， 进程允许最大阻塞 53.5MB 虚拟内存
 */
#define MAXIMUM_LOGSAVE_COUNT       (30000)

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
    if (write(file->fd_, buf, count) <= 0) {
        return -1;
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

        /* 没有发生日期切换 且 该文件内容记载没有超过限制行数, 则直接复用该文件 */
        if (file->filest_.day == currst->day &&
                file->filest_.month == currst->month &&
                file->filest_.day == currst->day &&
                file->line_count_ < MAXIMUM_LOGFILE_LINE) {
            return file;
        }

        /* 切换日志文件， 并不回收日志对象指针，仅仅是关闭文件描述符即可 */
        log__close_file(file);
    } while (0);

    char pedir[MAXPATH];
    posix__getpedir2(pedir, sizeof(pedir));
    posix__getpename2(pename, cchof(pename));


    /* 日志发件发生新建或任何形式的文件切换 */
    posix__sprintf(name, cchof(name), "%s_%04u%02u%02u_%02u%02u%02u.log", module,
            currst->year, currst->month, currst->day, currst->hour, currst->minute, currst->second);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s"POSIX__DIR_SYMBOL_STR, pedir, pename );
    posix__pmkdir(path);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s"POSIX__DIR_SYMBOL_STR"%s", pedir, pename, name);
    retval = log__create_file(file, path);
    if (retval >= 0) {
        memcpy(&file->filest_, currst, sizeof ( posix__systime_t));
        posix__strcpy(file->module_, cchof(file->module_), module);
        file->line_count_ = 0;
    } else {
        /* bug fixed:
         * 如果文件创建失败, 则需要移除链表节点 */
        list_del(&file->link_);
        free(file);
        file = NULL;
    }

    return file;
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
#if _WIN32
		printf("%s", logstr);
#else
        if (level == kLogLevel_Error) {
            write(STDERR_FILENO, logstr, cb); /* 2 */
        }else{
            write(STDOUT_FILENO, logstr, cb); /* 1 */
        }
#endif
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
#if _WIN32
            if (!list_empty(&__log_async.items_)) {
				InterlockedDecrement((volatile LONG *)&__log_async.pendding_);
                node = list_first_entry(&__log_async.items_, log__async_node_t, link_);
                if (node) {
                    list_del(&node->link_);
                }
            }
#else
            if (NULL != (node = list_first_entry_or_null(&__log_async.items_, log__async_node_t, link_))) {
                __sync_sub_and_fetch(&__log_async.pendding_, 1);
                list_del(&node->link_);
            }
#endif
            posix__pthread_mutex_unlock(&__log_async.lock_);

            if (node) {
                log__printf(node->module_, node->level_, node->target_, &node->logst_, node->logstr_, (int) strlen(node->logstr_));
            }
        } while (node);
    }

    /* 如果异步线程退出，则可能导致 log__save 内存堆积, 因为此时无法准确进行日志记录， 因此投递系统记录 */
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

posix__atomic_initial_declare_variable(__inited__) = POSIX__ATOMIC_INIT_TODO;

int log__init() {
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

void log__write(const char *module, enum log__levels level, int target, const char *format, ...) {
    va_list ap;
    char logstr[MAXIMUM_LOG_BUFFER_SIZE];
    posix__systime_t currst;

    if (log__init() < 0 || !format || level >= kLogLevel_Maximum || level < 0) {
        return;
    }

    posix__localtime(&currst);

    va_start(ap, format);
    log__format_string(level, posix__gettid(), format, ap, &currst, logstr, cchof(logstr));
    va_end(ap);

    if (!module) {
        char pename[MAXPATH], *p;
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
    int index;

    if (log__init() < 0 || !format || level >= kLogLevel_Maximum ) {
        return;
    }

    /* thread safe in count less or equal to 50 */
    if (posix__atomic_inc(&__log_async.pendding_) >= (MAXIMUM_LOGSAVE_COUNT - MAXIMUM_NUMBEROF_CONCURRENT_THREADS)) {
        posix__atomic_dec(&__log_async.pendding_);
        return;
    }

    /* atomic increase index */
#if _WIN32
    index = InterlockedIncrement(( LONG volatile *)&__log_async.misc_index);
#else
    index = __sync_fetch_and_add(&__log_async.misc_index, 1);
#endif
    node = (log__async_node_t *)&__log_async.misc_memory[index % MAXIMUM_LOGSAVE_COUNT];
    node->target_ = target;
    node->level_ = level;
    posix__localtime(&node->logst_);

    if (module) {
        posix__strcpy(node->module_, cchof(node->module_), module);
    } else {
        char pename[MAXPATH], *p;
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