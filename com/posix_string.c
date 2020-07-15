#include <assert.h>
#include <ctype.h>

#include "compiler.h"
#include "posix_string.h"

#if _WIN32
#include <Windows.h>
#else
#include <wchar.h>
#endif

int posix__strisdigit(const char *str, int len)
{
    int i;
    if (len <= 0 || !str) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (!isdigit(str[i])) {
            return -1;
        }
    }
    return 0;
}

const char *posix__strerror()
{
#if _WIN32
    char errmsg[128];
    static char errout[128];
    DWORD d = GetLastError();
    DWORD chs;
    chs = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errmsg, 128, 0);
    if (0 == chs) {
        posix__strcpy(errout, cchof(errout), "syscall fatal.");
    } else {
        posix__strcpy(errout, cchof(errout), errmsg);
    }
    return errout;
#else
    return strerror(errno);
#endif
}

const char *posix__strerror2(char *estr)
{
#if _WIN32
    if (!estr) {
		return NULL;
    }

    char errmsg[128];
    DWORD d = GetLastError();
    DWORD chs;
    chs = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, d, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errmsg, 128, 0);
    if (0 == chs) {
        posix__strcpy(estr, 128, "syscall fatal.");
    } else {
        posix__strcpy(estr, 128, errmsg);
    }
    return estr;
#else
    return strerror(errno);
#endif
}

char *posix__strncpy(char *target, uint32_t cch, const char *src, uint32_t cnt)
{
#if _WIN32
    errno_t e;
    if (!target || !src) {
        return NULL;
    }

    e = strncpy_s(target, cch, src, cnt);
    return ( (0 == e) ? target : NULL);
#else
    uint32_t cpyoff;

    assert(target);
    assert(src);
    assert(cnt < cch);

    cpyoff = 0;

    /* 到达拷贝指定长度 || src 达到杠零*/
    while ((src[cpyoff] != 0) && (cpyoff < cnt)) {
        assert(cpyoff < cch - 1);
        target[cpyoff] = src[cpyoff];
        cpyoff++;
    }
    target[cpyoff] = 0;
    return target;
#endif
}

wchar_t *posix__wcsncpy(wchar_t *target, uint32_t cch, const wchar_t *src, uint32_t cnt)
{
#if _WIN32
    errno_t e = wcsncpy_s(target, cch, src, cnt);
    return ( (0 == e) ? target : NULL);
#else
    /* GBU 的 strncpy 处理拷贝后自动追加杠零总是存在问题， glibc 又不支持 strlcpy， 因此这里采用自定义的非安全操作  */
    /* return wcsncpy(target, src, cnt); */
    uint32_t cpyoff;

    assert(target);
    assert(src);
    assert(cch > 0);

    cpyoff = 0;

    while ((cpyoff < cnt) && (src[cpyoff] != 0)) {
        assert((cpyoff + 1) < cch);
        target[cpyoff] = src[cpyoff];
        ++cpyoff;
    }
    target[cpyoff] = 0;
    return target;
#endif
}

char *posix__strtok(char *s, const char *delim, char **save_ptr)
{
#if _WIN32
    return strtok_s(
#else
    return strtok_r(
#endif
            s, delim, save_ptr);
}

wchar_t *posix__wcstok(wchar_t *s, const wchar_t *delim, wchar_t **save_ptr)
{
#if _WIN32
    return wcstok_s(s, delim, save_ptr);
#else
    return wcstok(s, delim, save_ptr);
#endif
}

char *posix__strcpy(char *target, uint32_t cch, const char *src)
{
#if _WIN32
    errno_t e;
    if (!target || !src) {
        return NULL;
    }
    e = strcpy_s(target, cch, src);
    return ( (0 == e) ? target : NULL);
#else
    /* 放弃使用 ISO-C 的部分不安全字符处理函数
     * 仿微软的搞法，模拟安全字符串拷贝函数
            return strcpy(target, src);
     *  */
    uint32_t cpyoff;

    assert(target);
    assert(src);
    assert(cch > 0);

    if (target == src) {
        return target;
    }

    cpyoff = 0;
    while (0 != src[cpyoff]) {
        assert(cpyoff < (cch - 1));
        target[cpyoff] = src[cpyoff];
        ++cpyoff;
    }
    target[cpyoff] = 0;
    return target;
#endif
}

wchar_t *posix__wcscpy(wchar_t *target, uint32_t cch, const wchar_t *src)
{
#if _WIN32
    wcscpy_s(target, cch, src);
    return target;
#else
    /* 因为安全问题弃用 ISO-c 的传统字符拷贝函数
     * return wcscpy(target, src); */
    uint32_t cpyoff;

    assert(target);
    assert(src);
    assert(cch > 0);

    cpyoff = 0;
    while (src[cpyoff] != 0) {
        assert((cpyoff + 1) < cch);
        target[cpyoff] = src[cpyoff];
        ++cpyoff;
    }

    target[cpyoff] = 0;
    return target;
#endif
}

char *posix__strdup(const char *src)
{
	if (!src) {
		return NULL;
	}
#if _WIN32
    return _strdup(src);
#else
    /* -D_POSIX_C_SOURCE >= 200809L 见 man*/
    return strdup(src);
#endif
}

wchar_t *posix__wcsdup(const wchar_t *src)
{
#if _WIN32
    return _wcsdup(src);
#else
    /* -D_POSIX_C_SOURCE >= 200809L 见 man*/
    return wcsdup(src);
#endif
}

char *posix__strcat(char *target, uint32_t cch, const char *src)
{
#if _WIN32
    errno_t e = strcat_s(target, cch, src);
    return ( (0 == e) ? target : NULL);
#else
    /* 因为安全问题弃用 ISO-c 的传统字符拷贝函数 */
    uint32_t cpyoff = 0, from_offset;

    assert(target);
    assert(src);

    while (target[cpyoff] != 0) {
        assert(cpyoff < cch);
        ++cpyoff;
    }

    from_offset = 0;
    while (src[cpyoff] != 0) {
        assert((cpyoff + 1) < cch);
        target[cpyoff] = src[cpyoff];
        ++cpyoff;
        ++from_offset;
    }
    target[cpyoff] = 0;
    return target;
#endif
}

wchar_t *posix__wcscat(wchar_t *target, uint32_t cch, const wchar_t *src)
{
#if _WIN32
    errno_t e = wcscat_s(target, cch, src);
    return ( (0 == e) ? target : NULL);
#else
    /* 因为安全问题弃用 ISO-c 的传统字符拷贝函数 */
    uint32_t copy_offset = 0, from_offset = 0;

    assert(target);
    assert(src);

    while (target[copy_offset] != 0) {
        assert(copy_offset < cch);
        ++copy_offset;
    }

    while (src[from_offset] != 0) {
        assert((copy_offset + 1) < cch);
        target[copy_offset] = src[from_offset];
        ++copy_offset;
        ++from_offset;
    }
    target[copy_offset] = 0;
    return target;
#endif
}

char *posix__strrev(char *src)
{
#if _WIN32
	if (!src) {
		return NULL;
	}
    return _strrev(src);
#else
    /* h指向s的头部 */
    char* h = src;
    char* t = src;
    char ch;

    if (!src) {
        return NULL;
    }

    /* t指向s的尾部 */
    while (*t++) {
        ;
    };

    t--; /* 与t++抵消 */
    t--; /* 回跳过结束符'\0' */

    /* 当h和t未重合时，交换它们所指向的字符 */
    while (h < t) {
        ch = *h;
        *h++ = *t; /* h向尾部移动 */
        *t-- = ch; /* t向头部移动 */
    }
    return ( src);
#endif
}

wchar_t *posix__wcsrev(wchar_t *src)
{
#if _WIN32
    return _wcsrev(src);
#else
    /* h指向s的头部 */
    wchar_t* h = src;
    wchar_t* t = src;
    wchar_t ch;

    /* t指向s的尾部 */
    while (*t++) {
    };
    t--; /* 与t++抵消 */
    t--; /* 回跳过结束符'\0' */

    /* 当h和t未重合时，交换它们所指向的字符 */
    while (h < t) {
        ch = *h;
        *h++ = *t; /* h向尾部移动 */
        *t-- = ch; /* t向头部移动 */
    }
    return ( src);
#endif
}

int posix__vsnprintf(char *const target, uint32_t cch, const char *format, va_list ap)
{
	if (!target || !format) {
		return -1;
	}
#if _WIN32
    return vsnprintf_s(target, cch, _TRUNCATE, format, ap);
#else
    return vsnprintf(target, cch, format, ap);
#endif
}

int posix__vsnwprintf(wchar_t * const target, uint32_t cch, const wchar_t *format, va_list ap)
{
#if _WIN32
    return _vsnwprintf_s(target, cch, _TRUNCATE, format, ap);
#else
    return vswprintf(target, cch, format, ap);
#endif
}

int posix__vsprintf(char *const target, uint32_t cch, const char *format, va_list ap)
{
	if (!target) {
		return -1;
	}
#if _WIN32
    return vsprintf_s(target, cch, format, ap);
#else
    return vsnprintf(target, cch, format, ap);
#endif
}

int posix__vswprintf(wchar_t * const target, uint32_t cch, const wchar_t *format, va_list ap)
{
#if _WIN32
    return vswprintf_s(target, cch, format, ap);
#else
    return vswprintf(target, cch, format, ap);
#endif
}

int posix__sprintf(char *const target, uint32_t cch, const char *fmt, ...)
{
    va_list ap;
	if (!target || !fmt) {
		return -1;
	}
    va_start(ap, fmt);
    int retval = posix__vsprintf(target, cch, fmt, ap);
    va_end(ap);
    return retval;
}

int posix__swprintf(wchar_t * const target, uint32_t cch, const wchar_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int retval = posix__vswprintf(target, cch, fmt, ap);
    va_end(ap);
    return retval;
}

int posix__strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

int posix__wcscmp(const wchar_t *s1, const wchar_t *s2)
{
    return wcscmp(s1, s2);
}

/****************************************************************************
 STRCASECMP() - Case-insensitive strcmp.
 *****************************************************************************/
int posix__strcasecmp(const char* s1, const char* s2)
{
    char c1, c2;
    do {
        c1 = *s1++;
        c2 = *s2++;
    } while (c1 && c2 && (tolower(c1) == tolower(c2)));

    return tolower(c1) - tolower(c2);
}

int posix__wcscasecmp(const wchar_t* s1, const wchar_t* s2)
{
    wchar_t c1, c2;
    do {
        c1 = *s1++;
        c2 = *s2++;
    } while (c1 && c2 && (tolower(c1) == tolower(c2)));

    return tolower(c1) - tolower(c2);
}

/****************************************************************************
 STRNCASECMP() - Case-insensitive strncmp.
 ****************************************************************************/
int posix__strncasecmp(const char* s1, const char* s2, uint32_t n)
{
    char c1, c2;

	if (!n) {
		return 0;
	}

    do {
        c1 = *s1++;
        c2 = *s2++;
    } while (--n && c1 && c2 && (tolower(c1) == tolower(c2)));

    return tolower(c1) - tolower(c2);
}

int posix__wcsncasecmp(const wchar_t* s1, const wchar_t* s2, uint32_t n)
{
    wchar_t c1, c2;

	if (!n) {
		return 0;
	}

    do {
        c1 = *s1++;
        c2 = *s2++;
    } while (--n && c1 && c2 && (tolower(c1) == tolower(c2)));

    return tolower(c1) - tolower(c2);
}

char *posix__strtrim(char *str)
{
    char *cursor;
    size_t i;

    cursor = str;
    if (!cursor) {
        return NULL;
    }

    /* automatic removal of invisible characters at the beginning of a string */
    while (*cursor && ( !isprint(*cursor) || (0x20 == *cursor) ) ) {
        ++cursor;
    }

    if (0 == *cursor) {
        return NULL;
    }

    for (i = strlen(cursor) - 1; i >= 0 ; i--) {
        /* when an invisible character or space is found,
            the following character of string is automatically ignored  */
        if (!isprint(cursor[i]) || 0x20 == cursor[i]) {
            cursor[i] = 0;
        } else {
            break;
        }
    }

    return cursor;
}

char *posix__strtrimdup(const char *origin)
{
    char *dup, *trimmed;

    if (!origin) {
        return NULL;
    }

#if _WIN32
	dup = _strdup(origin);
#else
    dup = strdup(origin);
#endif
    if (!dup) {
        return NULL;
    }

    trimmed = posix__strtrim(dup);
    if (!trimmed) {
        free(dup);
        return NULL;
    }

    if (dup != trimmed) {
        strcpy(dup, trimmed);
    }
    return dup;
}
