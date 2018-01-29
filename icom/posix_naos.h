#if !defined POSIX_NAOS_H
#define POSIX_NAOS_H

/*
 * posix_naos.h 定義一些于操作系统无关的工具函数
 * anderson 2017-05-08
 */

#include "compiler.h"

/*
 * ipv4tou ipv4tos 方法， 执行IPv4在字符串和32位无符号整形之间的转换， 可以指定大端或者小端
 */
__extern__
uint32_t posix__ipv4tou(const char *ipv4str, enum byte_order_t byte_order);
__extern__
char *posix__ipv4tos(uint32_t ip, char * ipstr, uint32_t cch);

/*
 * chord32 / chrod16 例程提供32位/16位的字节序转换操作
 *  */
__extern__
uint32_t posix__chord32( uint32_t value);
__extern__
uint16_t posix__chord16( uint16_t value);

#endif