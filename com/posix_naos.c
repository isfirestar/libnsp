#include "posix_naos.h"
#include "compiler.h"
#include "posix_string.h"
#include "posix_atomic.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

char *posix__ipv4tos(uint32_t ip, char * ipstr, uint32_t cch)
{
    unsigned char ipByte[4];
    char seg[4][4];
    int i;

    if (!ipstr || 0 == cch) {
        return NULL;
    }

    for (i = 0; i < 4; i++) {
        ipByte[i] = (unsigned char) (ip & 0xFF);
        ip >>= 8;
        posix__sprintf(seg[i], sizeof ( seg[i]), "%u", ipByte[i]);
    }

    posix__sprintf(ipstr, cch, "%s.%s.%s.%s", seg[3], seg[2], seg[1], seg[0]);
    return ipstr;
}

uint32_t posix__ipv4tou(const char *ipv4str, enum byte_order_t method)
{
    static const int BIT_MOV_FOR_LITTLE_ENDIAN[4] = {24, 16, 8, 0};
    static const int BIT_MOV_FOR_BIG_ENDIAN[4] = {0, 8, 16, 24};
    char *p;
    unsigned long byteValue;
    unsigned long ipv4Digit;
    char *nextToken;
    int i;
    char *Tmp;
    size_t sourceTextLengtchCch;

	if (!ipv4str) {
		return 0;
	}

    sourceTextLengtchCch = strlen(ipv4str);
    if (0 == sourceTextLengtchCch) {
        return 0;
    }

    ipv4Digit = 0;
    i = 0;

    Tmp = (char *) malloc(sourceTextLengtchCch + 1);
    if (!Tmp) {
        return 0;
    }
    posix__strcpy(Tmp, (int) (sourceTextLengtchCch + 1), ipv4str);

#if _WIN32
    nextToken = NULL;
	while (NULL != (p = strtok_s(nextToken ? NULL : Tmp, ".", &nextToken)) && i < 4) {
        byteValue = strtoul(p, NULL, 10);
        ipv4Digit |= byteValue << (kByteOrder_LittleEndian == method ? BIT_MOV_FOR_LITTLE_ENDIAN : BIT_MOV_FOR_BIG_ENDIAN)[i++];
    }
#else
    p = strtok_r(Tmp, ".", &nextToken);
    while (p) {
        byteValue = strtoul(p, NULL, 10);
        ipv4Digit |= byteValue << (kByteOrder_LittleEndian == method ? BIT_MOV_FOR_LITTLE_ENDIAN : BIT_MOV_FOR_BIG_ENDIAN)[i++];
        p = strtok_r(nextToken, ".", &nextToken);
    }
#endif

    free(Tmp);
    return ipv4Digit;
}

uint32_t posix__chord32(uint32_t value)
{
    uint32_t dst = 0;
    int i;

    for (i = 0; i < sizeof ( value); i++) {
        dst |= ((value >> (i * BITS_P_BYTE)) & 0xFF);
        dst <<= ((i) < (sizeof ( value) - 1) ? BITS_P_BYTE : (0));
    }
    return dst;
}

uint16_t posix__chord16(uint16_t value)
{
    uint16_t dst = 0;
    int i;

    for (i = 0; i < sizeof ( value); i++) {
        dst |= ((value >> (i * BITS_P_BYTE)) & 0xFF);
        dst <<= ((i) < (sizeof ( value) - 1) ? BITS_P_BYTE : (0));
    }
    return dst;
}

boolean_t posix__is_effective_address_v4(const char *ipstr)
{
    const char *cursor;
    int i, j, k;
    char segm[4][4];
    boolean_t success;

    if (!ipstr) {
        return NO;
    }

    success = YES;
    cursor = ipstr;
    i = j = k = 0;
    memset(segm, 0, sizeof(segm));

    while (*cursor) {
        if ((INET_ADDRSTRLEN - 1) == i) { /* 255.255.255.255. */
            success = NO;
            break;
        }

        if (k > 3) {  /* 192.168.1.0.1 */
            success = NO;
            break;
        }

        if (*cursor == '.' ) {
            if (0 == segm[k]) { /* .192.168.2.2 or 192..168.0.1 */
                success = NO;
                break;
            }

            if (atoi(segm[k]) > MAX_UINT8) { /* 256.1.1.0 */
                success = NO;
                break;
            }

            cursor++;
            i++;
            k++;
            j = 0;
            continue;
        }

        if (*cursor >= '0' && *cursor <= '9' ) {
            if (j >= 3) {  /* 1922.0.0.1 */
                success = NO;
                break;
            }
            segm[k][j] = *cursor;
            cursor++;
            i++;
            j++;
            continue;
        }

        /* any other characters */
        success = NO;
        break;
    }

    if (success) {
        if ( 3 != k ) { /* 192.168 */
            return NO;
        }

        if (0 == j) {   /* 192.168.0. */
            return NO;
        }
    }

    return success;
}
