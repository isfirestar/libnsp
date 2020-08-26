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

#if _WIN32

#define posix__strcpy(dest, size, src)	strcpy_s(dest, size, src)
#define posix__strncpy(dest, size, src, n) strncpy_s(dest, size, src, n)
#define posix__strcat(dest, size, src)	strcat_s(dest, size, src)
#define posix__strtok(str, delim, saveptr) strtok_s(str, delim, saveptr)
#define posix__strdup(s) _strdup(s)
#define posix__strcasecmp(s1, s2) _stricmp(s1, s2)
#define posix__strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#define posix__vsprintf(str, size, format, ap)	vsprintf_s(str, size, format, ap)
#define posix__vsnprintf(str, size, format, ap)	vsnprintf_s(str, size, _TRUNCATE, format, ap);
#define posix__sprintf(str, size, format, arg, ...) sprintf_s(str, size, format, ##arg)

#define posix__wcscat(dest, size, src)	wcscat_s(dest, size, src)
#define posix__wcscpy(dest, size, src)	wcscpy_s(dest, size, src)
#define posix__wcsncpy(dest, size, src, n) wcsncpy_s(dest, size, src, n)
#define posix__wcstok(wcs, delim, saveptr) wcstok_s(wcs, delim, saveptr)
#define posix__wcsdup(s) _wcsdup(s)
#define posix__wcscasecmp(s1, s2) _wcsicmp(s1, s2)
#define posix__wcsncasecmp(s1, s2, n) _wcsnicmp(s1, s2, n)
#define posix__vswprintf(wcs, maxlen, format, arg) vswprintf_s(wcs, maxlen, format, arg)
#define posix__vsnwprintf(wcs, maxlen, format, arg) _vsnwprintf_s(wcs, maxlen, _TRUNCATE, format, arg);
#define posix__swprintf(wcs, maxlen, format, arg, ...) swprintf_s(wcs, maxlen, format, #arg)


#else

#define posix__strcpy(dest, size, src)	strcpy(dest, src)
#define posix__strncpy(dest, size, src, n) strncpy(dest, src, n)
#define posix__strcat(dest, size, src)	strcat(dest, src)
#define posix__strtok(str, delim, saveptr) strtok_r(str, delim, saveptr)
#define posix__strdup(s) strdup(s) /* -D_POSIX_C_SOURCE >= 200809L */
#define posix__strcasecmp(s1, s2) strcasecmp(s1, s2)
#define posix__strncasecmp(s1, s2, n) strncasecmp(s1, s2, n)
#define posix__vsprintf(str, size, format, ap)	vsnprintf(str, size, format, ap)
#define posix__vsnprintf(str, size, format, ap)	vsnprintf(str, size, format, ap)
#define posix__sprintf(str, size, format, arg...) sprintf(str, format, ##arg)

#define posix__wcscpy(dest, size, src)	wcscpy(dest, src)
#define posix__wcsncpy(dest, size, src, n) wcsncpy(dest, src, n)
#define posix__wcscat(dest, size, src)	wcscat(dest, src)
#define posix__wcstok(wcs, delim, saveptr) wcstok(wcs, delim, saveptr)
#define posix__wcsdup(s) wcsdup(s)
#define posix__wcscasecmp(s1, s2) wcscasecmp(s1, s2)
#define posix__wcsncasecmp(s1, s2, n) wcsncasecmp(s1, s2, n)
#define posix__vswprintf(wcs, maxlen, format, arg) vswprintf(wcs, maxlen, format, arg)
#define posix__vsnwprintf(wcs, maxlen, format, arg) vswprintf(wcs, maxlen, format, arg)
#define posix__swprintf(wcs, maxlen, format, arg...) swprintf(wcs, maxlen, format, #arg)

#endif

#define posix__strcmp(s1, s2) strcmp(s1, s2)
#define posix__wcscmp(s1, s2) wcscmp(s1, s2)

#endif
