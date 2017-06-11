#include "posix_naos.h"
#include "compiler.h"
#include "posix_string.h"
#include "posix_atomic.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

char *posix__ipv4tos(uint32_t ip, char * ipstr, uint32_t cch) {
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

uint32_t posix__ipv4tou(const char *ipv4str, enum byte_order_t method) {
    static const int BIT_MOV_FOR_LITTLE_ENDIAN[4] = {24, 16, 8, 0};
    static const int BIT_MOV_FOR_BIG_ENDIAN[4] = {0, 8, 16, 24};
    char *p;
    unsigned long byteValue;
    unsigned long ipv4Digit;
#if _WIN32
    char *nextToken;
#endif
    int i;
    char *Tmp;
    size_t sourceTextLengtchCch;

    if (!ipv4str) return 0;

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
    posix__strcpy(Tmp, sourceTextLengtchCch + 1, ipv4str);
#if _WIN32
    nextToken = NULL;
    while (NULL != (p = strtok_s(nextToken ? NULL : Tmp, ".", &nextToken)) && i < 4) {
#else
    p = strtok(Tmp, ".");
    while (p) {
#endif
        byteValue = strtoul(p, NULL, 10);
        ipv4Digit |= byteValue << (kByteOrder_LittleEndian == method ? BIT_MOV_FOR_LITTLE_ENDIAN : BIT_MOV_FOR_BIG_ENDIAN)[i++];

#if !_WIN32
        p = strtok(NULL, ".");
#endif
    }

    free(Tmp);
    return ipv4Digit;
}

uint32_t posix__chord32(uint32_t value) {
    uint32_t dst = 0;
    int i;

    for (i = 0; i < sizeof ( value); i++) {
        dst |= ((value >> (i * BITS_P_BYTE)) & 0xFF);
        dst <<= ((i) < (sizeof ( value) - 1) ? BITS_P_BYTE : (0));
    }
    return dst;
}

uint16_t posix__chord16(uint16_t value) {
    uint16_t dst = 0;
    int i;

    for (i = 0; i < sizeof ( value); i++) {
        dst |= ((value >> (i * BITS_P_BYTE)) & 0xFF);
        dst <<= ((i) < (sizeof ( value) - 1) ? BITS_P_BYTE : (0));
    }
    return dst;
}