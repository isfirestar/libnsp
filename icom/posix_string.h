#if !defined POSIX_STRING_H
#define POSIX_STRING_H

#include "compiler.h"

#include <stdarg.h>

PORTABLEAPI(int) posix__strisdigit( const char *str, int len);

PORTABLEAPI(const char *) posix__strerror();
PORTABLEAPI(const char *) posix__strerror2(char *estr);

PORTABLEAPI(char *) posix__strrev(char *src);
PORTABLEAPI(wchar_t *) posix__wcsrev(wchar_t *src);

PORTABLEAPI(char *) posix__strtrim(char *str);
PORTABLEAPI(char *) posix__strtrimdup(const char *origin); /* the caller is always responsible to free the return pointer when not NULL */

PORTABLEAPI(char *) posix__strncpy(char *target, uint32_t cch, const char *src, uint32_t cnt);
PORTABLEAPI(wchar_t *) posix__wcsncpy(wchar_t *target, uint32_t cch, const wchar_t *src, uint32_t cnt);
PORTABLEAPI(char *) posix__strcpy(char *target, uint32_t cch, const char *src);
PORTABLEAPI(wchar_t *) posix__wcscpy(wchar_t *target, uint32_t cch, const wchar_t *src);
PORTABLEAPI(char *) posix__strcat(char *target, uint32_t cch, const char *src);
PORTABLEAPI(wchar_t *) posix__wcscat(wchar_t *target, uint32_t cch, const wchar_t *src);
PORTABLEAPI(int) posix__sprintf(char *const target, uint32_t cch, const char *fmt, ...);
PORTABLEAPI(int) posix__swprintf(wchar_t * const target, uint32_t cch, const wchar_t *fmt, ...);
PORTABLEAPI(int) posix__strcasecmp(const char* s1, const char* s2);
PORTABLEAPI(int) posix__wcscasecmp(const wchar_t* s1, const wchar_t* s2);
PORTABLEAPI(int) posix__strncasecmp(const char* s1, const char* s2, uint32_t n);
PORTABLEAPI(int) posix__wcsncasecmp(const wchar_t* s1, const wchar_t* s2, uint32_t n);
PORTABLEAPI(int) posix__strcmp(const char *s1, const char *s2);
PORTABLEAPI(int) posix__wcscmp(const wchar_t *s1, const wchar_t *s2);
PORTABLEAPI(int) posix__vsnprintf(char *const target, uint32_t cch, const char *format, va_list ap);
PORTABLEAPI(int) posix__vsnwprintf(wchar_t * const target, uint32_t cch, const wchar_t *format, va_list ap);
PORTABLEAPI(int) posix__vsprintf(char *const target, uint32_t cch, const char *format, va_list ap);
PORTABLEAPI(int) posix__vswprintf(wchar_t * const target, uint32_t cch, const wchar_t *format, va_list ap);

PORTABLEAPI(char *) posix__strtok(char *s, const char *delim, char **save_ptr);
PORTABLEAPI(wchar_t *) posix__wcstok(wchar_t *s, const wchar_t *delim, wchar_t **save_ptr);

PORTABLEAPI(char *) posix__strdup(const char *src);
PORTABLEAPI(wchar_t *) posix__wcsdup(const wchar_t *src);

/* [man 3 strncpy] The strncpy() function is similar, except that at most n bytes of src are copied.
	Warning: If there is no null byte among the first n bytes of src, the string placed in dest will not be null-terminated.

	[C standard]  If count is reached before the entire array src was copied, the resulting character array is not null-terminated.
 If, after copying the terminating null character from src, count is not reached,
 additional null characters are written to dest until the total of count characters have been written.*/

#if _WIN32

#define portable__strncpy(dest, maxlen, src, n) strncpy_s(dest, maxlen, src, n)
#define portable__wcsncpy(dest, maxlen, src, n) wcsncpy_s(dest, maxlen, src, n)
#define portable__strcpy(dest, maxlen, src)	strcpy_s(dest, maxlen, src)
#define portable__wcscpy(dest, maxlen, src)	wcscpy_s(dest, maxlen, src)
#define portable__strcat(dest, maxlen, src)	strcat_s(dest, maxlen, src)
#define portable__wcscat(dest, maxlen, src)	wcscat_s(dest, maxlen, src)
#define portable__vsnprintf(str, maxlen, format, ap)	vsnprintf_s(str, maxlen, _TRUNCATE, format, ap);
#define portable__vsnwprintf(wcs, maxlen, format, arg) _vsnwprintf_s(wcs, maxlen, _TRUNCATE, format, arg);
#define portable__vsprintf(str, maxlen, format, ap)	vsprintf_s(str, maxlen, format, ap)
#define portable__vswprintf(wcs, maxlen, format, arg) vswprintf_s(wcs, maxlen, format, arg)
#define portable__sprintf(str, maxlen, format, ...) sprintf_s(str, maxlen, format, ##__VA_ARGS__)
#define portable__swprintf(wcs, maxlen, format, ...) swprintf_s(wcs, maxlen, format, ##__VA_ARGS__)
#define portable__strcasecmp(s1, s2) _stricmp(s1, s2)
#define portable__wcscasecmp(s1, s2) _wcsicmp(s1, s2)
#define portable__strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#define portable__wcsncasecmp(s1, s2, n) _wcsnicmp(s1, s2, n)
#define portable__strtok(str, delim, saveptr) strtok_s(str, delim, saveptr)
#define portable__wcstok(wcs, delim, saveptr) wcstok_s(wcs, delim, saveptr)
#define portable__strdup(s) _strdup(s)
#define portable__wcsdup(s) _wcsdup(s)

#else

#define portable__strncpy(dest, maxlen, src, n) strncpy(dest, src, n)
#define portable__wcsncpy(dest, maxlen, src, n) wcsncpy(dest, src, n)
#define portable__strcpy(dest, maxlen, src)	strcpy(dest, src)
#define portable__wcscpy(dest, maxlen, src)	wcscpy(dest, src)
#define portable__strcat(dest, maxlen, src)	strcat(dest, src)
#define portable__wcscat(dest, maxlen, src)	wcscat(dest, src)
#define portable__vsnprintf(str, maxlen, format, ap)	vsnprintf(str, maxlen, format, ap)
#define portable__vsnwprintf(wcs, maxlen, format, arg) vswprintf(wcs, maxlen, format, arg)
#define portable__vsprintf(str, maxlen, format, ap)	vsnprintf(str, maxlen, format, ap)
#define portable__vswprintf(wcs, maxlen, format, arg) vswprintf(wcs, maxlen, format, arg)
#define portable__sprintf(str, maxlen, format, arg...) sprintf(str, format, ##arg)
#define portable__swprintf(wcs, maxlen, format, arg...) swprintf(wcs, maxlen, format, ##arg)
#define portable__strcasecmp(s1, s2) strcasecmp(s1, s2)
#define portable__wcscasecmp(s1, s2) wcscasecmp(s1, s2)
#define portable__strncasecmp(s1, s2, n) strncasecmp(s1, s2, n)
#define portable__wcsncasecmp(s1, s2, n) wcsncasecmp(s1, s2, n)
#define portable__strtok(str, delim, saveptr) strtok_r(str, delim, saveptr)
#define portable__wcstok(wcs, delim, saveptr) wcstok(wcs, delim, saveptr)
#define portable__strdup(s) strdup(s) /* -D_POSIX_C_SOURCE >= 200809L */
#define portable__wcsdup(s) wcsdup(s)

#endif

#define portable__strcmp(s1, s2) strcmp(s1, s2)
#define portable__wcscmp(s1, s2) wcscmp(s1, s2)

#endif
