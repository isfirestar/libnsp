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

#define posix__strcpy(dest, maxlen, src)	strcpy_s(dest, maxlen, src)

/* [man 3 strncpy] The strncpy() function is similar, except that at most n bytes of src are copied.
	Warning: If there is no null byte among the first n bytes of src, the string placed in dest will not be null-terminated.

	[C standard]  If count is reached before the entire array src was copied, the resulting character array is not null-terminated.
 If, after copying the terminating null character from src, count is not reached,
 additional null characters are written to dest until the total of count characters have been written.*/
#define posix__strncpy(dest, maxlen, src, n) strncpy_s(dest, maxlen, src, n)
#define posix__strcat(dest, maxlen, src)	strcat_s(dest, maxlen, src)
#define posix__strtok(str, delim, saveptr) strtok_s(str, delim, saveptr)
#define posix__strdup(s) _strdup(s)
#define posix__strcasecmp(s1, s2) _stricmp(s1, s2)
#define posix__strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#define posix__vsprintf(str, maxlen, format, ap)	vsprintf_s(str, maxlen, format, ap)
#define posix__vsnprintf(str, maxlen, format, ap)	vsnprintf_s(str, maxlen, _TRUNCATE, format, ap);
#define posix__sprintf(str, maxlen, format, ...) sprintf_s(str, maxlen, format, ##__VA_ARGS__)

#define posix__wcscat(dest, maxlen, src)	wcscat_s(dest, maxlen, src)
#define posix__wcscpy(dest, maxlen, src)	wcscpy_s(dest, maxlen, src)
#define posix__wcsncpy(dest, maxlen, src, n) wcsncpy_s(dest, maxlen, src, n)
#define posix__wcstok(wcs, delim, saveptr) wcstok_s(wcs, delim, saveptr)
#define posix__wcsdup(s) _wcsdup(s)
#define posix__wcscasecmp(s1, s2) _wcsicmp(s1, s2)
#define posix__wcsncasecmp(s1, s2, n) _wcsnicmp(s1, s2, n)
#define posix__vswprintf(wcs, maxlen, format, arg) vswprintf_s(wcs, maxlen, format, arg)
#define posix__vsnwprintf(wcs, maxlen, format, arg) _vsnwprintf_s(wcs, maxlen, _TRUNCATE, format, arg);
#define posix__swprintf(wcs, maxlen, format, ...) swprintf_s(wcs, maxlen, format, ##__VA_ARGS__)

#else

#define posix__strcpy(dest, maxlen, src)	strcpy(dest, src)
/* [man 3 strncpy] The strncpy() function is similar, except that at most n bytes of src are copied.
	Warning: If there is no null byte among the first n bytes of src, the string placed in dest will not be null-terminated.

	[C standard]  If count is reached before the entire array src was copied, the resulting character array is not null-terminated.
 If, after copying the terminating null character from src, count is not reached,
 additional null characters are written to dest until the total of count characters have been written.*/
#define posix__strncpy(dest, maxlen, src, n) strncpy(dest, src, n)
#define posix__strcat(dest, maxlen, src)	strcat(dest, src)
#define posix__strtok(str, delim, saveptr) strtok_r(str, delim, saveptr)
#define posix__strdup(s) strdup(s) /* -D_POSIX_C_SOURCE >= 200809L */
#define posix__strcasecmp(s1, s2) strcasecmp(s1, s2)
#define posix__strncasecmp(s1, s2, n) strncasecmp(s1, s2, n)
#define posix__vsprintf(str, maxlen, format, ap)	vsnprintf(str, maxlen, format, ap)
#define posix__vsnprintf(str, maxlen, format, ap)	vsnprintf(str, maxlen, format, ap)
#define posix__sprintf(str, maxlen, format, arg...) sprintf(str, format, ##arg)

#define posix__wcscpy(dest, maxlen, src)	wcscpy(dest, src)
#define posix__wcsncpy(dest, maxlen, src, n) wcsncpy(dest, src, n)
#define posix__wcscat(dest, maxlen, src)	wcscat(dest, src)
#define posix__wcstok(wcs, delim, saveptr) wcstok(wcs, delim, saveptr)
#define posix__wcsdup(s) wcsdup(s)
#define posix__wcscasecmp(s1, s2) wcscasecmp(s1, s2)
#define posix__wcsncasecmp(s1, s2, n) wcsncasecmp(s1, s2, n)
#define posix__vswprintf(wcs, maxlen, format, arg) vswprintf(wcs, maxlen, format, arg)
#define posix__vsnwprintf(wcs, maxlen, format, arg) vswprintf(wcs, maxlen, format, arg)
#define posix__swprintf(wcs, maxlen, format, arg...) swprintf(wcs, maxlen, format, ##arg)

#endif

#define posix__strcmp(s1, s2) strcmp(s1, s2)
#define posix__wcscmp(s1, s2) wcscmp(s1, s2)

#endif
