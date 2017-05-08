#if !defined POSIX_NAOS_H
#define POSIX_NAOS_H

/*
 * posix_naos.h ���xһЩ�ڲ���ϵͳ�޹صĹ��ߺ���
 * anderson 2017-05-08
 */

#include "compiler.h"

/*
 * ipv4tou ipv4tos ������ ִ��IPv4���ַ�����32λ�޷�������֮���ת���� ����ָ����˻���С��
 */
__extern__
uint32_t posix__ipv4tou(const char *ipv4str, enum byte_order_t byte_order);
__extern__
char *posix__ipv4tos(uint32_t ip, char * ipstr, uint32_t cch);

/*
 * chord32 / chrod16 �����ṩ32λ/16λ���ֽ���ת������
 *  */
__extern__
uint32_t posix__chord32( uint32_t value);
__extern__
uint16_t posix__chord16( uint16_t value);

#endif