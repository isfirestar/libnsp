#if !defined POSIX_NAOS_H
#define POSIX_NAOS_H

/*
 * posix_naos.h Define some OS-independent functions
 * anderson 2017-05-08
 */

#include "compiler.h"

/* Switching IPv4 representation method between Dotted-Decimal-Notation and integer
 */
__interface__
uint32_t posix__ipv4tou(const char *ipstr, enum byte_order_t byte_order);
__interface__
char *posix__ipv4tos(uint32_t ip, char *ipstr, uint32_t cch);

/* the same as htonl(3)/ntohl(3)/ntohs(3)/htons(3)
 */
__interface__
uint32_t posix__chord32( uint32_t value);
__interface__
uint16_t posix__chord16( uint16_t value);

/* verfiy the IP address string */
__interface__
boolean_t posix__is_effective_address_v4(const char *ipstr);

#endif
