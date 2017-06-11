#include "crc32.h"
#include "posix_atomic.h"

uint32_t crc32_table[256];

#define CRC32_POLY (0xEDB88320L)

void make_crc32_table() {
    uint32_t c;
    int i = 0;
    int bit = 0;

    for (i = 0; i < 256; i++) {
        c = (uint32_t) i;
        for (bit = 0; bit < 8; bit++) {
            if (c & 1) {
                c = (c >> 1)^ CRC32_POLY;
            } else {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
}

static int is_crc32_inited = 0;

uint32_t crc32(uint32_t crc, const unsigned char *string, uint32_t size) {
    if (1 == posix__atomic_inc(&is_crc32_inited)) {
        make_crc32_table();
    } else {
        posix__atomic_dec(&is_crc32_inited);
    }

    crc ^= 0xFFFFFFFF;
    while (size--)
        crc = (crc >> 8)^(crc32_table[(crc & 0xff) ^ *string++]);

    return crc ^ 0xFFFFFFFF;
}