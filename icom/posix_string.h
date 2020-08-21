#if !defined POSIX_STRING_H
#define POSIX_STRING_H

#include "compiler.h"

#include <stdarg.h>

PORTABLEAPI(char *) posix__trim(char *src);
PORTABLEAPI(int) posix__strisdigit( const char *str, int len);
PORTABLEAPI(const char *) posix__strerror();
PORTABLEAPI(const char *) posix__strerror2(char *estr);
PORTABLEAPI(char *) posix__strtok(char *s, const char *delim, char **save_ptr);
PORTABLEAPI(wchar_t *) posix__wcstok(wchar_t *s, const wchar_t *delim, wchar_t **save_ptr);
PORTABLEAPI(char *) posix__strcpy(char *target, uint32_t cch, const char *src);
PORTABLEAPI(wchar_t *) posix__wcscpy(wchar_t *target, uint32_t cch, const wchar_t *src);
PORTABLEAPI(char *) posix__strncpy(char *target, uint32_t cch, const char *src, uint32_t cnt);
PORTABLEAPI(wchar_t *) posix__wcsncpy(wchar_t *target, uint32_t cch, const wchar_t *src, uint32_t cnt);
PORTABLEAPI(char *) posix__strdup(const char *src);
PORTABLEAPI(wchar_t *) posix__wcsdup(const wchar_t *src);
PORTABLEAPI(char *) posix__strcat(char *target, uint32_t cch, const char *src);
PORTABLEAPI(wchar_t *) posix__wcscat(wchar_t *target, uint32_t cch, const wchar_t *src);
PORTABLEAPI(char *) posix__strrev(char *src);
PORTABLEAPI(wchar_t *) posix__wcsrev(wchar_t *src);
PORTABLEAPI(int) posix__vsnprintf(char *const target, uint32_t cch, const char *format, va_list ap);
PORTABLEAPI(int) posix__vsnwprintf(wchar_t *const target, uint32_t cch, const wchar_t *format, va_list ap);
PORTABLEAPI(int) posix__vsprintf(char *const target, uint32_t cch, const char *format, va_list ap);
PORTABLEAPI(int) posix__vswprintf(wchar_t *const target, uint32_t cch, const wchar_t *format, va_list ap);
PORTABLEAPI(int) posix__sprintf(char *const target, uint32_t cch, const char *fmt, ...);
PORTABLEAPI(int) posix__swprintf(wchar_t *const target, uint32_t cch, const wchar_t *fmt, ...);
PORTABLEAPI(int) posix__strcmp(const char *s1, const char *s2);
PORTABLEAPI(int) posix__wcscmp(const wchar_t *s1, const wchar_t *s2);
PORTABLEAPI(int) posix__strcasecmp(const char *s1, const char *s2);
PORTABLEAPI(int) posix__wcscasecmp(const wchar_t* s1, const wchar_t* s2);
PORTABLEAPI(int) posix__strncasecmp(const char* s1, const char* s2, uint32_t n);
PORTABLEAPI(int) posix__wcsncasecmp(const wchar_t* s1, const wchar_t* s2, uint32_t n);
PORTABLEAPI(char *) posix__strtrim(char *str);
PORTABLEAPI(char *) posix__strtrimdup(const char *origin); /* the caller is always responsible to free the return pointer when not NULL */

#endif
