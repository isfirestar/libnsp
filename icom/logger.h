#if !defined LOGGER_H
#define LOGGER_H

#include "compiler.h"

/*
 *	bug:
 *	1. Log file switching rows are 5000 lines with a minimum switching interval of 1 second.
 *		If more than 5000 rows of logs are continuously output in one second, the log information will be saved in the same log file
 */

enum log__levels {
    kLogLevel_Info = 0,
    kLogLevel_Warning,
    kLogLevel_Error,
    kLogLevel_Fatal,
    kLogLevel_Trace,
    kLogLevel_Maximum,
};

#define kLogTarget_Filesystem   (1)
#define kLogTarget_Stdout       (2)
#define	kLogTarget_Sysmesg      (4)

__interface__ int log__init();
#define log_init() log__init()
__interface__ int log__init2(const char *rootdir);
__interface__ void log__write(const char *module, enum log__levels level, int target, const char *format, ...);
__interface__ void log__save(const char *module, enum log__levels level, int target, const char *format, ...);
__interface__ void log__flush();

/* Maximum allowable specified log module name length */
#define  LOG_MODULE_NAME_LEN   (128)

/* Maximum allowable single log write data length  */
#define  MAXIMUM_LOG_BUFFER_SIZE  (2048)

#if _WIN32
#define ECHO(module, fmt, arg, ...) log__save(module, kLogLevel_Info, kLogTarget_Stdout | kLogTarget_Filesystem, fmt, ##arg)
#define ALERT(module, fmt, arg, ...) log__save(module, kLogLevel_Warning, kLogTarget_Stdout | kLogTarget_Filesystem, fmt, ##arg)
#define FATAL(module, fmt, arg, ...) log__save(module, kLogLevel_Error, kLogTarget_Stdout | kLogTarget_Filesystem, fmt, ##arg)
#define TRACE(module, fmt, arg, ...) log__save(module, kLogLevel_Error, kLogTarget_Filesystem, fmt, ##arg)
#else
#define ECHO(module, fmt, arg...) log__save(module, kLogLevel_Info, kLogTarget_Stdout | kLogTarget_Filesystem, fmt, ##arg)
#define ALERT(module, fmt, arg...) log__save(module, kLogLevel_Warning, kLogTarget_Stdout | kLogTarget_Filesystem, fmt, ##arg)
#define FATAL(module, fmt, arg...) log__save(module, kLogLevel_Error, kLogTarget_Stdout | kLogTarget_Filesystem, fmt, ##arg)
#define TRACE(module, fmt, arg...) log__save(module, kLogLevel_Trace, kLogTarget_Filesystem, fmt, ##arg)
#endif

#endif
