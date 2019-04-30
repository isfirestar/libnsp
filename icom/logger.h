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

__extern__ int log__init();
__extern__ void log__write(const char *module, enum log__levels level, int target, const char *format, ...);
__extern__ void log__save(const char *module, enum log__levels level, int target, const char *format, ...);

/* Maximum allowable specified log module name length */
#define  LOG_MODULE_NAME_LEN   (128)

/* Maximum allowable single log write data length  */
#define  MAXIMUM_LOG_BUFFER_SIZE  (2048)

#endif