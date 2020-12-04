#include "logger.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "compiler.h"

#include "clist.h"
#include "posix_string.h"
#include "posix_thread.h"
#include "posix_time.h"
#include "posix_wait.h"
#include "posix_ifos.h"
#include "posix_atomic.h"

/* the upper limit of the number of rows in a log file  */
#define  MAXIMUM_LOGFILE_LINE    (5000)

/* maximum asynchronous cache */
#define MAXIMUM_LOGSAVE_COUNT       (500)

static const char *LOG__LEVEL_TXT[] = {
    "info", "warning", "error", "fatal", "trace"
};

struct log_file_descriptor {
    struct list_head file_link;
    file_descriptor_t fd;
    posix__systime_t timestamp;
    int line_count_;
    char module[LOG_MODULE_NAME_LEN];
} ;

static LIST_HEAD(__log__file_head); /* list<struct log_file_descriptor> */
static posix__pthread_mutex_t __log_file_lock;
static char __log_root_directory[MAXPATH] = { 0 };

struct log_async_node {
    struct list_head link;
    char logstr[MAXIMUM_LOG_BUFFER_SIZE];
    int target;
    posix__systime_t timestamp;
    enum log__levels level;
    char module[LOG_MODULE_NAME_LEN];
};

struct log_async_context {
    int pending;
    struct list_head idle;
    struct list_head busy;
    posix__pthread_mutex_t lock;
    posix__pthread_t async_thread;
    posix__waitable_handle_t notify;
    struct log_async_node *misc_memory;
};

static struct log_async_context __log_async;

static
int log__create_file(struct log_file_descriptor *file, const char *path)
{
    if (!file || !path) {
        return -EINVAL;
    }

    return posix__file_open(path, FF_RDACCESS | FF_WRACCESS | FF_CREATE_ALWAYS, 0644, &file->fd);
}

static
void log__close_file(struct log_file_descriptor *file)
{
    if (file) {
        posix__file_close(file->fd);
        file->fd = INVALID_FILE_DESCRIPTOR;
    }
}

static
int log__fwrite(struct log_file_descriptor *file, const void *buf, int count)
{
    int retval;

    retval = posix__file_write(file->fd, buf, count);
    if ( retval > 0) { /* need posix__file_flush(file->fd) ? */
        file->line_count_++;
    }

    return retval;
}

static
struct log_file_descriptor *log__attach(const posix__systime_t *currst, const char *module)
{
    char name[128], path[512], pename[128];
    int retval;
    struct list_head *pos;
    struct log_file_descriptor *file;

    if (!currst || !module) {
        return NULL;
    }

    file = NULL;

    list_for_each(pos, &__log__file_head) {
        file = containing_record(pos, struct log_file_descriptor, file_link);
        if (0 == posix__strcasecmp(module, file->module)) {
            break;
        }
        file = NULL;
    }

    do {
        /* empty object, it means this module is a new one */
        if (!file) {
            if (NULL == (file = malloc(sizeof ( struct log_file_descriptor)))) {
                return NULL;
            }
            memset(file, 0, sizeof ( struct log_file_descriptor));
            file->fd = INVALID_FILE_DESCRIPTOR;
            list_add_tail(&file->file_link, &__log__file_head);
            break;
        }

        if ((int) file->fd < 0) {
            break;
        }

        /* If no date switch occurs and the file content record does not exceed the limit number of rows,
            the file is reused directly.  */
        if (file->timestamp.day == currst->day &&
                file->timestamp.month == currst->month &&
                file->timestamp.day == currst->day &&
                file->line_count_ < MAXIMUM_LOGFILE_LINE) {
            return file;
        }

        /* Switching the log file does not reclaim the log object pointer, just closing the file descriptor  */
        log__close_file(file);
    } while (0);

    posix__getpename2(pename, cchof(pename));

    /* New or any form of file switching occurs in log posts  */
    posix__sprintf(name, cchof(name), "%s_%04u%02u%02u_%02u%02u%02u.log",
		module, currst->year, currst->month, currst->day, currst->hour, currst->minute, currst->second);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s"POSIX__DIR_SYMBOL_STR, __log_root_directory, pename );
    posix__pmkdir(path);
    posix__sprintf(path, cchof(path), "%s"POSIX__DIR_SYMBOL_STR"log"POSIX__DIR_SYMBOL_STR"%s"POSIX__DIR_SYMBOL_STR"%s", __log_root_directory, pename, name);
    retval = log__create_file(file, path);
    if (retval >= 0) {
        memcpy(&file->timestamp, currst, sizeof ( posix__systime_t));
        posix__strcpy(file->module, cchof(file->module), module);
        file->line_count_ = 0;
    } else {
        /* If file creation fails, the linked list node needs to be removed  */
        list_del_init(&file->file_link);
        free(file);
        file = NULL;
    }

    return file;
}

static
void log__printf(const char *module, enum log__levels level, int target, const posix__systime_t *currst, const char* logstr, int cb)
{
    struct log_file_descriptor *fileptr;

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
        posix__file_write(level == kLogLevel_Error ? STDERR_FILENO : STDOUT_FILENO, logstr, cb);
    }

    if (target & kLogTarget_Sysmesg) {
        posix__syslog(logstr);
    }
}

static
char *log__format_string(enum log__levels level, int tid, const char* format, va_list ap, const posix__systime_t *currst, char *logstr, int cch)
{
    int pos, n, c;
    char *p;

    if (level >= kLogLevel_Maximum || !currst || !format || !logstr) {
        return NULL;
    }

    p = logstr;
    pos = 0;
	pos += posix__sprintf(&p[pos], cch - pos, "%02u:%02u:%02u %04u ", currst->hour, currst->minute, currst->second, (unsigned int)(currst->low / 10000));
    pos += posix__sprintf(&p[pos], cch - pos, "%s ", LOG__LEVEL_TXT[level]);
    pos += posix__sprintf(&p[pos], cch - pos, "%04X # ", tid);

    c = cch - pos - 1 - sizeof(POSIX__EOL);
	n = vsnprintf(&p[pos], c, format, ap);
#if _WIN32
	if (n < 0) {
#else
    if (n >= c) {
#endif
		pos = MAXIMUM_LOG_BUFFER_SIZE - 1 - sizeof(POSIX__EOL);
	} else {
		pos += n;
	}
    pos += posix__sprintf(&p[pos], cch - pos, "%s", POSIX__EOL);
	p[pos] = 0;

    return p;
}

static
struct log_async_node *log__get_idle_node()
{
    struct log_async_node *node;

    node = NULL;

    posix__pthread_mutex_lock(&__log_async.lock);

    do {
        if (list_empty(&__log_async.idle)) {
            break;
        }

        node = list_first_entry(&__log_async.idle, struct log_async_node, link);
        assert(node);
        list_del_init(&node->link);
        /* do NOT add node into busy queue now, data are not ready.
        list_add_tail(&node->link, &__log_async.busy); */
    } while(0);
    posix__pthread_mutex_unlock(&__log_async.lock);

    return node;
}

static
void log__recycle_async_node(struct log_async_node *node)
{
    assert(node);
    posix__pthread_mutex_lock(&__log_async.lock);
    list_del_init(&node->link);
    list_add_tail(&node->link, &__log_async.idle);
    posix__pthread_mutex_unlock(&__log_async.lock);
}

static
void *log__async_proc(void *argv)
{
    while (posix__waitfor_waitable_handle(&__log_async.notify, 10 * 1000) >= 0) {
        log__flush();
    }

    posix__syslog("nsplog asynchronous thread has been terminated.");
    return NULL;
}

static
int log__async_init()
{
    int misc_memory_size;
    int i;

    __log_async.pending = 0;
    misc_memory_size = MAXIMUM_LOGSAVE_COUNT * sizeof(struct log_async_node);

    __log_async.misc_memory = (struct log_async_node *)malloc(misc_memory_size);
    if (!__log_async.misc_memory) {
        return -ENOMEM;
    }
    memset(__log_async.misc_memory, 0, misc_memory_size);

    INIT_LIST_HEAD(&__log_async.idle);
    INIT_LIST_HEAD(&__log_async.busy);

    posix__pthread_mutex_init(&__log_async.lock);

    posix__init_synchronous_waitable_handle(&__log_async.notify);

    /* all miscellaneous memory nodes are inital adding to free list */
    for (i = 0; i < MAXIMUM_LOGSAVE_COUNT; i++) {
        list_add_tail(&__log_async.misc_memory[i].link, &__log_async.idle);
    }

    if (posix__pthread_create(&__log_async.async_thread, &log__async_proc, NULL) < 0) {
        posix__pthread_mutex_release(&__log_async.lock);
        posix__uninit_waitable_handle(&__log_async.notify);
        free(__log_async.misc_memory);
        __log_async.misc_memory = NULL;
        return -1;
    }
    return 0;
}

static
void log__change_rootdir(const char *rootdir)
{
	size_t pos, i;

    if (0 != __log_root_directory[0]) {
        return;
    }

    if (!rootdir) {
        posix__getpedir2(__log_root_directory, sizeof(__log_root_directory));
        return;
	}

    posix__strcpy(__log_root_directory, cchof(__log_root_directory), rootdir);
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
}

static int __log__init()
{
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

PORTABLEIMPL(int) log__init()
{
	return log__init2(NULL);
}

PORTABLEIMPL(int) log__init2(const char *rootdir)
{
    log__change_rootdir(rootdir);
	return __log__init();
}

PORTABLEIMPL(void) log__write(const char *module, enum log__levels level, int target, const char *format, ...)
{
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

PORTABLEIMPL(void) log__save(const char *module, enum log__levels level, int target, const char *format, ...)
{
    va_list ap;
    struct log_async_node *node;
    char pename[MAXPATH], *p;

    if (log__init() < 0 || !format || level >= kLogLevel_Maximum || level < 0) {
        return;
    }

    /* securt check for the maximum pending amount */
    if (posix__atomic_inc(&__log_async.pending) >= MAXIMUM_LOGSAVE_COUNT) {
        posix__atomic_dec(&__log_async.pending);
        return;
    }

    /* hash node at index of memory block */
    if ( NULL == (node = log__get_idle_node()) ) {
        return;
    }
    node->target = target;
    node->level = level;
    posix__localtime(&node->timestamp);

    if (module) {
        posix__strcpy(node->module, cchof(node->module), module);
    } else {
        p = posix__getpename2(pename, sizeof(pename));
        if (p) {
            posix__strcpy(node->module, cchof(node->module), pename);
        }
    }

    va_start(ap, format);
    log__format_string(node->level, posix__gettid(), format, ap, &node->timestamp, node->logstr, sizeof (node->logstr));
    va_end(ap);

    posix__pthread_mutex_lock(&__log_async.lock);
    list_add_tail(&node->link, &__log_async.busy);
    posix__pthread_mutex_unlock(&__log_async.lock);
    posix__sig_waitable_handle(&__log_async.notify);
}

PORTABLEIMPL(void) log__flush()
{
    struct log_async_node *node;

    do {
        node = NULL;

        posix__pthread_mutex_lock(&__log_async.lock);
        if (!list_empty(&__log_async.busy)) {
            posix__atomic_dec(&__log_async.pending);
            node = list_first_entry(&__log_async.busy, struct log_async_node, link);
            assert(node);
        }
        posix__pthread_mutex_unlock(&__log_async.lock);

        if (node) {
            log__printf(node->module, node->level, node->target, &node->timestamp, node->logstr, (int) strlen(node->logstr));

            /* recycle node from busy queue to idle list */
            log__recycle_async_node(node);
        }
    } while (node);
}
