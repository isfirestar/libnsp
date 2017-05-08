#ifndef POSIX_TYPES_H
#define POSIX_TYPES_H

#include "compiler.h"

#if !defined POSIX__EOL
#if _WIN32
#define POSIX__EOL              "\r\n"
#define POSIX__DIR_SYMBOL       '\\'
#define POSIX__DIR_SYMBOL_STR   "\\"
#else
#define POSIX__EOL              "\n"
#define POSIX__DIR_SYMBOL       '/'
#define POSIX__DIR_SYMBOL_STR   "/"
#endif
#endif

#if !defined __cplusplus
typedef int posix__boolean_t;      /* use boolean type means only true/false*/
#else
typedef bool posix__boolean_t;
#endif

#if !defined posix__true
#define posix__true ((posix__boolean_t)1)
#endif

#if !defined posix__false
#define posix__false ((posix__boolean_t)0)
#endif

#if !defined posix__ipv4_length
#define posix__ipv4_length          (16)
#endif

#endif /* POSIX_TYPES_H */

