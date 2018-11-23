#if !defined LOGGER_H
#define LOGGER_H

#include "compiler.h"

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

__extern__
int log__init();
__extern__
void log__write(const char *module, enum log__levels level, int target, const char *format, ...);
__extern__
void log__save(const char *module, enum log__levels level, int target, const char *format, ...);

/* 最大允许指定日志模块名称长度 */
#define  LOG_MODULE_NAME_LEN   (128)

/* 最大允许单次日志写入数据长度 */
#define  MAXIMUM_LOG_BUFFER_SIZE  (2048)

#endif