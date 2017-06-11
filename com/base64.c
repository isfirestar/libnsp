#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const unsigned char BASE64_ENCODE_TABLE[] = {
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
};

int base64__encode(const char *input, int incb, char *output, int *outcb) {
    int cnt_symbol_add;
    int src_len, dst_len;
    int page;
    char *src_buffer, *dst_buffer;
    int i;
    int t;
    int k;

    if (!input || incb <= 0 || (!output && !outcb)) {
        return -EINVAL;
    }

    /* 补‘=’个数 */
    cnt_symbol_add = 0;
    if (0 != (incb % 3)) {
        cnt_symbol_add = 3 - (incb % 3);
    }

    /* 原文长度补全，并且计算分页数 */
    src_len = incb + cnt_symbol_add;
    page = src_len / 3;
    dst_len = page * 4;
    if (!output) {
        *outcb = dst_len;
        return 0;
    } else {
        if (*outcb < dst_len) {
            *outcb = dst_len;
            return 0;
        }
    }

    src_buffer = (char *) malloc(src_len);
    if (!src_buffer) {
        return -1;
    }
    memset(src_buffer, 0, src_len);
    memcpy(src_buffer, input, incb);

    dst_buffer = (char *) malloc(dst_len);
    if (!dst_buffer) {
        free(src_buffer);
        return -1;
    }

    for (i = 0, t = 0; i < page; i++, t++) {
        unsigned char src[3];
        memcpy(src, &src_buffer[i * 3], 3);

        unsigned char dst[4];
        /* 原文0字节高6位=>目标0字节低6位 */
        dst[0] = (src[0] >> 2);

        /* 原文0字节低2位=>目标1字节高2位 原文1字节高4位=>目标1字节低4位 */
        dst[1] = ((src[0] & 3) << 4) | (src[1] >> 4);

        /* 原文1字节低4位=>目标2字节高4位 原文2字节高2位=>目标2字节低2位 */
        dst[2] = ((src[1] & 0xf) << 2) | (src[2] >> 6);

        /* 原文2字节低6位=>目标3字节满6位 */
        dst[3] = (src[2] & 0x3f);

        for (k = 0; k < 4; k++) {
            dst[k] = *(BASE64_ENCODE_TABLE + dst[k]);
        }

        memcpy(&dst_buffer[t * 4], dst, 4);
    }

    /* 最后的0数据字节用'='替代 */
    for (k = 0; k < cnt_symbol_add; k++) {
        dst_buffer[dst_len - (1 + k)] = '=';
    }

    memcpy(output, dst_buffer, dst_len);
    /* output.assign(dst_buffer, dst_len); */

    free(src_buffer);
    free(dst_buffer);

    return 0;
}

static const signed char BASE64_DECODE_TABLE[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

int base64__decode(const char *input, int incb, char *output, int *outcb) {
    int page;
    int dst_len;
    char *dst_buffer;
    int i, t, j;

    if (!input || incb <= 0 || incb % 4 != 0 || !outcb) {
        return -1;
    }

    page = incb / 4;
    dst_len = page * 3;
    if (!output) {
        *outcb = dst_len;
        return 0;
    } else {
        if (*outcb < dst_len) {
            *outcb = dst_len;
            return 0;
        }
    }

    dst_buffer = (char *) malloc(dst_len);
    if (!dst_buffer) {
        return -1;
    }

    for (i = 0, t = 0; i < page; i++, t++) {
        unsigned char src[4];
        unsigned char dst[3] = {0};

        memcpy(src, &input[i * 4], 4);
        for (j = 0; j < 4; j++) {
            if (src[j] >= sizeof ( BASE64_DECODE_TABLE) / sizeof ( BASE64_DECODE_TABLE[0])) {
                free(dst_buffer);
                return -1;
            }
            if ('=' != src[j]) {
                src[j] = *(BASE64_DECODE_TABLE + src[j]);
                if (-1 == src[j]) {
                    free(dst_buffer);
                    return -1;
                }
            }
        }

        /* 密文0字节满6位=>原文0字节高6位 */
        dst[0] = src[0];
        dst[0] <<= 2;

        /* 密文1字节(4,5)位=>原文0字节低2位 */
        dst[0] |= (src[1] >> 4);

        /* 密文1字节(0,1,2,3)位=>原文1字节高4位 */
        dst[1] = (src[1] & 0xf);
        dst[1] <<= 4;

        if ('=' != src[2]) {
            /* 密文2字节(2,3,4,5)位=>原文1字节低4位 */
            dst[1] |= (src[2] >> 2);
            /* 密文2字节(0,1)位=>原文2字节高2位 */
            dst[2] = (src[2] & 3);
            dst[2] <<= 6;
        } else {
            /* 从返回长度中去除'=' */
            dst_len--;
        }

        if ('=' != src[3]) {
            /* 密文3字节满6位=>原文2字节低6位 */
            dst[2] |= src[3];
        } else {
            /* 从返回长度中去除'=' */
            dst_len--;
        }

        memcpy(&dst_buffer[t * 3], dst, 3);
    }

    memcpy(output, dst_buffer, dst_len);
    free(dst_buffer);
    return 0;
}