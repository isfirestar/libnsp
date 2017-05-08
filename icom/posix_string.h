#if !defined POSIX_STRING_H
#define POSIX_STRING_H

#include "compiler.h"

#include <stdarg.h>

__extern__
char *posix__trim(char *src);
__extern__
int posix__strisdigit( const char *str, int len);
__extern__
const char *posix__strerror();
__extern__
char *posix__strtok(char *s, const char *delim, char **save_ptr);
__extern__
wchar_t *posix__wcstok(wchar_t *s, const wchar_t *delim, wchar_t **save_ptr);
__extern__
char *posix__strcpy(char *target, uint32_t cch, const char *src);
__extern__
wchar_t *posix__wcscpy(wchar_t *target, uint32_t cch, const wchar_t *src);
__extern__
char *posix__strncpy(char *target, uint32_t cch, const char *src, uint32_t cnt);
__extern__
wchar_t *posix__wcsncpy(wchar_t *target, uint32_t cch, const wchar_t *src, uint32_t cnt);
__extern__
char *posix__strdup(const char *src);
__extern__
wchar_t *posix__wcsdup(const wchar_t *src);
__extern__
char *posix__strcat(char *target, uint32_t cch, const char *src);
__extern__
wchar_t *posix__wcscat(wchar_t *target, uint32_t cch, const wchar_t *src);
__extern__
char *posix__strrev(char *src);
__extern__
wchar_t *posix__wcsrev(wchar_t *src);
__extern__
int posix__vsnprintf(char *const target, uint32_t cch, const char *format, va_list ap);
__extern__
int posix__vsnwprintf(wchar_t *const target, uint32_t cch, const wchar_t *format, va_list ap);
__extern__
int posix__vsprintf(char *const target, uint32_t cch, const char *format, va_list ap);
__extern__
int posix__vswprintf(wchar_t *const target, uint32_t cch, const wchar_t *format, va_list ap);
__extern__
int posix__sprintf(char *const target, uint32_t cch, const char *fmt, ...);
__extern__
int posix__swprintf(wchar_t *const target, uint32_t cch, const wchar_t *fmt, ...);
__extern__
int posix__strcmp(const char *s1, const char *s2);
__extern__
int posix__wcscmp(const wchar_t *s1, const wchar_t *s2);
__extern__
int posix__strcasecmp(const char *s1, const char *s2);
__extern__
int posix__wcscasecmp(const wchar_t* s1, const wchar_t* s2);
__extern__
int posix__strncasecmp(const char* s1, const char* s2, uint32_t n);
__extern__
int posix__wcsncasecmp(const wchar_t* s1, const wchar_t* s2, uint32_t n);

/* �����ַ��� @s, ȥ��ǰ��ո񣬷��ش������ַ�����ʼָ��
 * ������:
 * ��Ȼ @s ������ԭʼָ�룬�����ַ���β���Ŀո񣬻ᱻ���������
 * �����Ҫ��������ԭʼ����Ӧ�ÿ���ʹ�� posix__strtrimcpy
*/
__extern__
const char *posix__strtrim(const char *s); 
__extern__
char *posix__strtrimcpy(char *target, uint32_t cch, const char *src); /* trim�����ַ��� @src ����뿽�� @target  */

#endif