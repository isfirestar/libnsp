#if !defined LOGGER_H
#define LOGGER_H

#include "compiler.h"

enum log__levels {
    kLogLevel_Info = 0,
    kLogLevel_Warning,
    kLogLevel_Error,
    kLogLevel_Trace,
    kLogLevel_Maximum,
};

enum log__targets {
    kLogTarget_Filesystem = 1,
    kLogTarget_Stdout = 2,
    kLogTraget_EmergentEvent = 4,
};

__extern__
void log__write(const char *module, enum log__levels level, enum log__targets target, const char *format, ...);
__extern__
void log__save(const char *module, enum log__levels level, enum log__targets target, const char *format, ...);

#endif