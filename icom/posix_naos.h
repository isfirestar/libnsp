#if !defined POSIX_NAOS_H
#define POSIX_NAOS_H

/*
 * posix_naos.h Define some OS-independent functions
 * anderson 2017-05-08
 */

#include "compiler.h"

/* Switching IPv4 representation method between Dotted-Decimal-Notation and integer 
 */
__extern__
uint32_t posix__ipv4tou(const char *ipv4str, enum byte_order_t byte_order);
__extern__
char *posix__ipv4tos(uint32_t ip, char * ipstr, uint32_t cch);

/* the same as htonl(3)/ntohl(3)/ntohs(3)/htons(3)
 */
__extern__
uint32_t posix__chord32( uint32_t value);
__extern__
uint16_t posix__chord16( uint16_t value); 

#endif