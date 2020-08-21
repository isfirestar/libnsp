#if !defined POSIX_NAOS_H
#define POSIX_NAOS_H

/*
 * posix_naos.h Define some OS-independent functions
 * anderson 2017-05-08
 */

#include "compiler.h"

/* Switching IPv4 representation method between Dotted-Decimal-Notation and integer
 */
PORTABLEAPI(uint32_t) posix__ipv4tou(const char *ipstr, enum byte_order_t byte_order);
PORTABLEAPI(char *) posix__ipv4tos(uint32_t ip, char *ipstr, uint32_t cch);

/* the same as htonl(3)/ntohl(3)/ntohs(3)/htons(3)
 */
PORTABLEAPI(uint32_t) posix__chord32( uint32_t value);
PORTABLEAPI(uint16_t) posix__chord16( uint16_t value);

/* verfiy the IP address string */
PORTABLEAPI(boolean_t) posix__is_effective_address_v4(const char *ipstr);

#endif
