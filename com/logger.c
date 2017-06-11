#include <stdlib.h>
#include <stdio.h>

#if _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdarg.h>

#include "logger.h"
#include "clist.h"

#include "posix_types.h"
#include "posix_string.h"
#include "posix_thread.h"
#include "posix_time.h"
#include "posix_wait.h"
#include "posix_ifos.h"


#define  MAXIMUM_LOGFILE_LINE  (5000)

/* 最大异步日志存储数量， 达到次数量， 将无法进行异步日志存储 */
#define MAXIMUM_LOGSAVE_COUNT       (300000)

static const char *LOG__LEVEL_TXT[] = {
    "info",
    "warning",
    "error",
    "trace",
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
static 
LIST_HEAD(__log__file_head); /* list<log__file_describe_t> */
static 
posix__pthread_mutex_t __log_file_lock;

typedef struct {
    struct list_head link_;
    char logstr_[MAXIMUM_LOG_BUFFER_SIZE];
    enum log__targets target_;
    posix__systime_t logst_;
    int tid_;
    char module_[LOG_MODULE_NAME_LEN];
} log__async_node_t;

static
long __log_async_save_cnt = 0;
static
LIST_HEAD(__log_async_head); /* list<log__async_node_t> */
static 
posix__pthread_mutex_t __log_async_lock;
static 
posix__pthread_t __log_async_thread = POSIX_PTHREAD_TYPE_INIT;
static
posix__waitable_handle_t __log_async_alert;

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
            file->fd_ = open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
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
    if (write(file->fd_, buf, count) < 0) {
        return -1;
    }
#endif
    
    /* posix__fflush(file->fd_) */
    file->line_count_++;
    return 0;
}

static
log__file_describe_t *log__attach(const posix__systime_t *currst, const char *module) {
    char name[128], path[MAXPATH];
    int retval;
    struct list_head *pos;
    log__file_describe_t *file;

    if (!currst || !module) return NULL;

    file = NULL;

    list_for_each(pos, &__log__file_head) {
        file = containing_record(pos, log__file_describe_t, link_);
        if (0 == posix__strcasecmp(module, file->module_)) {
            break;
        }
        file = NULL;
    }

    do {
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

        if (file->filest_.day == currst->day &&
                file->filest_.month == currst->month &&
                file->filest_.day == currst->day &&
                file->line_count_ < MAXIMUM_LOGFILE_LINE) {
            return file;
        }
        
        /* 切换日志文件， 并不回收日志对象指针，仅仅是关闭文件描述符即可 */
        log__close_file(file);
    } while (0);

    posix__sprintf(name, cchof(name), "%s_%04u%02u%02u_%02u%02u%02u.log", module,
            currst->year, currst->month, currst->day, currst->hour, currst->minute, currst->second);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR, posix__getpedir());
    posix__mkdir(path);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s", posix__getpedir(), name);
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
void log__printf(const char *module, enum log__targets target, const posix__systime_t *currst, const char* logstr, int cb) {
    log__file_describe_t *fileptr;

    if (target & kLogTarget_Filesystem) {
        posix__pthread_mutex_lock(&__log_file_lock);
        fileptr = log__attach(currst, module);
        posix__pthread_mutex_unlock(&__log_file_lock);
        if (fileptr) {
            log__fwrite(fileptr, logstr, cb);
        }
    }

    if (target & kLogTarget_Stdout) {
        printf("%s", logstr);
    }
}

static
char *log__format_string(enum log__levels level, int tid, const char* format, va_list ap, const posix__systime_t *currst, char *logstr, int cch) {
    int pos;
    char *p;

    if (level >= kLogLevel_Maximum || !currst || !format || !logstr) return NULL;

    p = logstr;
    pos = 0;
    pos += posix__sprintf(&p[pos], cch - pos, "%02u:%02u:%02u %04u ", currst->hour, currst->minute, currst->second, currst->milli_second);
    pos += posix__sprintf(&p[pos], cch - pos, "%s ", LOG__LEVEL_TXT[level]);
    pos += posix__sprintf(&p[pos], cch - pos, "%04X # ", tid);
    pos += posix__vsnprintf(&p[pos], cch - pos, format, ap);
    pos += posix__sprintf(&p[pos], cch - pos, "%s", POSIX__EOL);

    return p;
}

static
void *log__asnyc_proc(void *argv) {
    log__async_node_t *node;

    while (posix__waitfor_waitable_handle(&__log_async_alert, 10 * 1000) >= 0) {
        do {
            node = NULL;

            posix__pthread_mutex_lock(&__log_async_lock);
#if _WIN32
            if (!list_empty(&__log_async_head)) {
                --__log_async_save_cnt;
                node = list_first_entry(&__log_async_head, log__async_node_t, link_);
                if (node) {
                    list_del(&node->link_);
                }
            }
#else
            if (NULL != (node = list_first_entry_or_null(&__log_async_head, log__async_node_t, link_))) {
                --__log_async_save_cnt;
                list_del(&node->link_);
            }
#endif
            posix__pthread_mutex_unlock(&__log_async_lock);

            if (node) {
                log__printf(node->module_, node->target_, &node->logst_, node->logstr_, strlen(node->logstr_));
                free(node);
            }
        } while (node);
    }

    return NULL;
}

static
int log__init() {
    if (0 == __log_async_thread.pid_) {
        posix__pthread_mutex_init(&__log_file_lock);
        posix__pthread_mutex_init(&__log_async_lock);
        posix__init_synchronous_waitable_handle(&__log_async_alert);

        if (posix__pthread_create(&__log_async_thread, &log__asnyc_proc, NULL) < 0) {
            posix__pthread_mutex_release(&__log_async_lock);
            posix__uninit_waitable_handle(&__log_async_alert);
            return -1;
        }
    }
    return 0;
}

void log__write(const char *module, enum log__levels level, int target, const char *format, ...) {
    va_list ap;
    char logstr[MAXIMUM_LOG_BUFFER_SIZE];
    posix__systime_t currst;

    if (log__init() < 0){
        return;
    }

    posix__localtime(&currst);

    va_start(ap, format);
    log__format_string(level, posix__gettid(), format, ap, &currst, logstr, cchof(logstr));
    va_end(ap);

    if (!module) {
        log__printf(posix__getpename(), target, &currst, logstr, strlen(logstr));
    } else {
        log__printf(module, target, &currst, logstr, strlen(logstr));
    }
}

void log__save(const char *module, enum log__levels level, int target, const char *format, ...) {
    va_list ap;
    log__async_node_t *node;

    if (log__init() < 0){
        return;
    }

    if (NULL == (node = (log__async_node_t *) malloc(sizeof ( log__async_node_t)))) {
        return;
    }
    node->target_ = target;
    posix__localtime(&node->logst_);
    posix__strcpy(node->module_, cchof(node->module_), module);

    va_start(ap, format);
    log__format_string(level, posix__gettid(), format, ap, &node->logst_, node->logstr_, sizeof(node->logstr_));
    va_end(ap);

    do {
        posix__pthread_mutex_lock(&__log_async_lock);
        if (__log_async_save_cnt < MAXIMUM_LOGSAVE_COUNT) {
            list_add_tail(&node->link_, &__log_async_head);
            ++__log_async_save_cnt;
            break;
        }

        free(node);
    } while (0);

    posix__pthread_mutex_unlock(&__log_async_lock);
    posix__sig_waitable_handle(&__log_async_alert);
}