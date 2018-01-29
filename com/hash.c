﻿#include "hash.h"
#include "posix_atomic.h"

#include <string.h>
#include <stdlib.h>

/*--------------------------------------------VFN1/VFN1a--------------------------------------------*/
#define VFN_PRIME32 ((uint32_t)16777619L)
#define VFN_PRIME64	((uint64_t)1099511628211ULL)

#define VFN_OFFSET_BASIS32		((uint32_t)2166136261L)
#define VFN_OFFSET_BASIS64		((uint64_t)14695981039346656037ULL)

uint32_t vfn1_h32(const unsigned char *key, int length) {
	uint32_t hash;
	int i;

	if ( !key || length <= 0 ) {
		return 0;
	}

	hash = VFN_OFFSET_BASIS32;
	for ( i = 0; i < length; i++ ) {
		hash *= VFN_PRIME32;
		hash ^= key[i];
	}
	return hash;
}

uint64_t vfn1_h64(const unsigned char *key, int length) {
	uint64_t hash;
	int i;

	if ( !key || length <= 0 ) {
		return 0;
	}

	hash = VFN_OFFSET_BASIS64;
	for ( i = 0; i < length; i++ ) {
		hash *= VFN_PRIME64;
		hash ^= key[i];
	}
	return hash;
}

uint32_t vfn1a_h32(const unsigned char *key, int length) {
	uint32_t hash;
	int i;

	if ( !key || length <= 0 ) {
		return 0;
	}

	hash = VFN_OFFSET_BASIS32;
	for ( i = 0; i < length; i++ ) {
		hash ^= key[i];
		hash *= VFN_PRIME32;
	}
	return hash;
}

uint64_t vfn1a_h64(const unsigned char *key, int length) {
	uint64_t hash;
	int i;

	if ( !key || length <= 0 ) {
		return 0;
	}

	hash = VFN_OFFSET_BASIS64;
	for ( i = 0; i < length; i++ ) {
		hash ^= key[i];
		hash *= VFN_PRIME64;
	}
	return hash;
}

/*--------------------------------------------CRC32--------------------------------------------*/
uint32_t crc32_table[256];
static int is_crc32_inited = 0;

#define CRC32_POLY (0xEDB88320L)

static void make_crc32_table() {
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

/*--------------------------------------------BASE64--------------------------------------------*/
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

/*--------------------------------------------MD5--------------------------------------------*/
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
        (a) += F ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
                                }
#define GG(a, b, c, d, x, s, ac) { \
        (a) += G ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
                                }
#define HH(a, b, c, d, x, s, ac) { \
        (a) += H ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
                                }
#define II(a, b, c, d, x, s, ac) { \
        (a) += I ((b), (c), (d)) + (x) + (uint32_t)(ac); \
        (a) = ROTATE_LEFT ((a), (s)); \
        (a) += (b); \
                                }

static
void MD5__Transform(uint32_t state[4], const uint8_t block[64]);
static
void MD5__Encode(uint8_t *output, const uint32_t *input, uint32_t len);
static
void MD5__Decode(uint32_t *output, const uint8_t *input, uint32_t len);
static
void MD5__memcpy(uint8_t* output, const uint8_t* input, uint32_t len);
static
void MD5__memset(uint8_t* output, int value, uint32_t len);

static
void MD5__Transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    MD5__Decode(x, block, 64);

    /* Round 1 */
    FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
    FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
    HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    MD5__memset((uint8_t*) x, 0, sizeof ( x));
}

static
void MD5__Encode(uint8_t *output, const uint32_t *input, uint32_t len) {
    uint32_t i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (uint8_t) (input[i] & 0xff);
        output[j + 1] = (uint8_t) ((input[i] >> 8) & 0xff);
        output[j + 2] = (uint8_t) ((input[i] >> 16) & 0xff);
        output[j + 3] = (uint8_t) ((input[i] >> 24) & 0xff);
    }
}

static
void MD5__Decode(uint32_t *output, const uint8_t *input, uint32_t len) {
    uint32_t i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((uint32_t) input[j]) | (((uint32_t) input[j + 1]) << 8) |
        (((uint32_t) input[j + 2]) << 16) | (((uint32_t) input[j + 3]) << 24);
}

static
void MD5__memcpy(uint8_t* output, const uint8_t* input, uint32_t len) {
    uint32_t i;

    for (i = 0; i < len; i++)
        output[i] = input[i];
}

static
void MD5__memset(uint8_t* output, int value, uint32_t len) {
    uint32_t i;

    for (i = 0; i < len; i++)
        ((char *) output)[i] = (char) value;
}

void MD5__Init(MD5_CTX *md5ctx) {
    md5ctx->count[0] = md5ctx->count[1] = 0;
    md5ctx->state[0] = 0x67452301;
    md5ctx->state[1] = 0xefcdab89;
    md5ctx->state[2] = 0x98badcfe;
    md5ctx->state[3] = 0x10325476;

    MD5__memset(md5ctx->PADDING, 0, sizeof ( md5ctx->PADDING));
    *md5ctx->PADDING = 0x80;
    /*PADDING = {
     *	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     *	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     *	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	};
     * */
}

void MD5__Update(MD5_CTX *md5ctx, const uint8_t *input, uint32_t inputLen) {
    uint32_t i, index, partLen;

    index = (uint32_t) ((md5ctx->count[0] >> 3) & 0x3F);

    if ((md5ctx->count[0] += ((uint32_t) inputLen << 3))
            < ((uint32_t) inputLen << 3))
        md5ctx->count[1]++;
    md5ctx->count[1] += ((uint32_t) inputLen >> 29);

    partLen = 64 - index;
    if (inputLen >= partLen) {
        MD5__memcpy((uint8_t*) & md5ctx->buffer[index],
                (uint8_t*) input, partLen);
        MD5__Transform(md5ctx->state, md5ctx->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5__Transform(md5ctx->state, &input[i]);

        index = 0;
    } else
        i = 0;

    MD5__memcpy((uint8_t*) & md5ctx->buffer[index], (uint8_t*) & input[i], inputLen - i);
}

void MD5__Final(MD5_CTX *md5ctx, uint8_t digest[16]) {
    uint8_t bits[8];
    uint32_t index, padLen;

    MD5__Encode(bits, md5ctx->count, 8);

    index = (uint32_t) ((md5ctx->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5__Update(md5ctx, md5ctx->PADDING, padLen);

    MD5__Update(md5ctx, bits, 8);
    MD5__Encode(digest, md5ctx->state, 16);

    MD5__memset((uint8_t*) md5ctx, 0, sizeof ( *md5ctx));
    MD5__Init(md5ctx);
}

/*--------------------------------------------DES--------------------------------------------*/
typedef char DES_ElemType;

/* 初始置换表IP */
static const int DES_IP_Table[64] = {
    58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7
};

/* 逆初始置换表IP^-1 */
static const int DES_IP_1_Table[64] = {
    40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25
};

/* 扩充置换表E */
static const int DES_E_Table[48] = {
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
    8, 9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1
};

/* 置换函数P */
static const int DES_P_Table[32] = {
    16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9, 19, 13, 30, 6, 22, 11, 4, 25
};

/* S盒 */
static const int DES_S[8][4][16] ={
    /* S1 */
    {
        { 14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
        { 0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
        { 4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
        { 15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}
    },
    /* S2 */
    {
        { 15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
        { 3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
        { 0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
        { 13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}
    },
    /* S3 */
    {
        { 10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
        { 13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
        { 13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
        { 1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}
    },
    /* S4 */
    {
        { 7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
        { 13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
        { 10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
        { 3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}
    },
    /* S5 */
    {
        { 2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
        { 14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
        { 4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
        { 11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}
    },
    /* S6 */
    {
        { 12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
        { 10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
        { 9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
        { 4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}
    },
    /* S7 */
    {
        { 4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
        { 13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
        { 1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
        { 6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}
    },
    /* S8 */
    {
        { 13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
        { 1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
        { 7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
        { 2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
    }
};

/* 置换选择1 */
static const int DES_PC_1[56] = {
    57, 49, 41, 33, 25, 17, 9, 1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27, 19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15, 7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 28, 20, 12, 4
};

/* 置换选择2 */
static const int DES_PC_2[48] = {
    14, 17, 11, 24, 1, 5, 3, 28, 15, 6, 21, 10,
    23, 19, 12, 4, 26, 8, 16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
}; /* 密钥压缩表 */

/* 对左移位数表 */
static const int DES_MOVE_TIMES[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

/* 字节转换成二进制 */
static void ByteToBit(DES_ElemType ch, DES_ElemType bit[8]) {
    int cnt;
    for (cnt = 0; cnt < 8; cnt++) {
        *(bit + cnt) = (ch >> cnt) & 1;
    }
}

/* 二进制转换成字节 */
static void BitToByte(DES_ElemType bit[8], DES_ElemType *ch) {
    int cnt;
    for (cnt = 0; cnt < 8; cnt++) {
        *ch |= *(bit + cnt) << cnt;
    }
}

/* 将长度为8的字符串转为二进制位串 */
static void Char8ToBit64(const DES_ElemType ch[8], DES_ElemType bit[64]) {
    int cnt;
    for (cnt = 0; cnt < 8; cnt++) {
        ByteToBit(*(ch + cnt), bit + (cnt << 3));
    }
}

/* 将二进制位串转为长度为8的字符串 */
static void Bit64ToChar8(DES_ElemType bit[64], DES_ElemType ch[8]) {
    int cnt;
    memset(ch, 0, 8);
    for (cnt = 0; cnt < 8; cnt++) {
        BitToByte(bit + (cnt << 3), ch + cnt);
    }
}

/* 密钥置换方法 */
static void DES_PC1_Transform(DES_ElemType key[64], DES_ElemType tempbts[56]) {
    int cnt;
    for (cnt = 0; cnt < 56; cnt++) {
        tempbts[cnt] = key[DES_PC_1[cnt] - 1];
    }
}

/* 循环左移 */
static void DES_ROL(DES_ElemType data[56], int time) {
    DES_ElemType temp[56];

    /* 保存将要循环移动到右边的位 */
    memcpy(temp, data, time);
    memcpy(temp + time, data + 28, time);

    /* 前28位移动 */
    memcpy(data, data + time, 28 - time);
    memcpy(data + 28 - time, temp, time);

    /* 后28位移动 */
    memcpy(data + 28, data + 28 + time, 28 - time);
    memcpy(data + 56 - time, temp + time, time);
}

/* 密钥扩展方法 */
static void DES_PC2_Transform(DES_ElemType key[56], DES_ElemType tempbts[48]) {
    int cnt;
    for (cnt = 0; cnt < 48; cnt++) {
        tempbts[cnt] = key[DES_PC_2[cnt] - 1];
    }
}

/* 生成子密钥 */
static void DES_MakeSubKeys(DES_ElemType key[64], DES_ElemType subKeys[16][48]) {
    DES_ElemType temp[56];
    int cnt;
    DES_PC1_Transform(key, temp); /* PC1置换 */
    for (cnt = 0; cnt < 16; cnt++) { /* 16轮跌代，产生16个子密钥 */
        DES_ROL(temp, DES_MOVE_TIMES[cnt]); /* 循环左移 */
        DES_PC2_Transform(temp, subKeys[cnt]); /* PC2置换，产生子密钥 */
    }
}

/* IP置换 */
static void DES_IP_Transform(DES_ElemType data[64]) {
    int cnt;
    DES_ElemType temp[64];
    for (cnt = 0; cnt < 64; cnt++) {
        temp[cnt] = data[DES_IP_Table[cnt] - 1];
    }
    memcpy(data, temp, 64);
}

/* IP逆置换 */
static void DES_IP_1_Transform(DES_ElemType data[64]) {
    int cnt;
    DES_ElemType temp[64];
    for (cnt = 0; cnt < 64; cnt++) {
        temp[cnt] = data[DES_IP_1_Table[cnt] - 1];
    }
    memcpy(data, temp, 64);
}

/* 扩展置换 */
static void DES_E_Transform(DES_ElemType data[48]) {
    int cnt;
    DES_ElemType temp[48];
    for (cnt = 0; cnt < 48; cnt++) {
        temp[cnt] = data[DES_E_Table[cnt] - 1];
    }
    memcpy(data, temp, 48);
}

/* P置换 */
static void DES_P_Transform(DES_ElemType data[32]) {
    int cnt;
    DES_ElemType temp[32];
    for (cnt = 0; cnt < 32; cnt++) {
        temp[cnt] = data[DES_P_Table[cnt] - 1];
    }
    memcpy(data, temp, 32);
}

/* 异或 */
static void DES_XOR(DES_ElemType R[48], DES_ElemType L[48], int count) {
    int cnt;
    for (cnt = 0; cnt < count; cnt++) {
        R[cnt] ^= L[cnt];
    }
}

/* 交换 */
static void DES_Swap(DES_ElemType left[32], DES_ElemType right[32]) {
    DES_ElemType temp[32];
    memcpy(temp, left, 32);
    memcpy(left, right, 32);
    memcpy(right, temp, 32);
}

/* S盒置换 */
static void DES_SBOX(DES_ElemType data[48]) {
    int cnt;
    int line, row, output;
    int cur1, cur2;
    for (cnt = 0; cnt < 8; cnt++) {
        cur1 = cnt * 6;
        cur2 = cnt << 2;

        /* 计算在S盒中的行与列 */
        line = (data[cur1] << 1) + data[cur1 + 5]; /* 每组第一位乘2加上第六位 */
        row = (data[cur1 + 1] << 3) + (data[cur1 + 2] << 2)
                + (data[cur1 + 3] << 1) + data[cur1 + 4]; /* 每组第二位乘8加上第三位乘4加上第四位乘2加上第五位 */
        output = DES_S[cnt][line][row];

        /* 化为2进制 */
        data[cur2] = (output & 0X08) >> 3;
        data[cur2 + 1] = (output & 0X04) >> 2;
        data[cur2 + 2] = (output & 0X02) >> 1;
        data[cur2 + 3] = output & 0x01;
    }
}

/* 加密单个分组 */
static void DES_EncryptBlock(const DES_ElemType plainBlock[8], DES_ElemType subKeys[16][48], DES_ElemType cipherBlock[8]) {
    DES_ElemType plainBits[64];
    DES_ElemType copyRight[48];
    int cnt;

    Char8ToBit64(plainBlock, plainBits);
    /* 初始置换（IP置换） */
    DES_IP_Transform(plainBits);

    /* 16轮迭代 */
    for (cnt = 0; cnt < 16; cnt++) {
        memcpy(copyRight, plainBits + 32, 32);
        /* 将右半部分进行扩展置换，从32位扩展到48位 */
        DES_E_Transform(copyRight);
        /* 将右半部分与子密钥进行异或操作 */
        DES_XOR(copyRight, subKeys[cnt], 48);
        /* 异或结果进入S盒，输出32位结果 */
        DES_SBOX(copyRight);
        /* P置换 */
        DES_P_Transform(copyRight);
        /* 将明文左半部分与右半部分进行异或 */
        DES_XOR(plainBits, copyRight, 32);
        if (cnt != 15) {
            /* 最终完成左右部的交换 */
            DES_Swap(plainBits, plainBits + 32);
        }
    }
    /* 逆初始置换（IP^1置换） */
    DES_IP_1_Transform(plainBits);
    Bit64ToChar8(plainBits, cipherBlock);
}

/* 解密单个分组 */
static void DES_DecryptBlock(const DES_ElemType cipherBlock[8], DES_ElemType subKeys[16][48], DES_ElemType plainBlock[8]) {
    DES_ElemType cipherBits[64];
    DES_ElemType copyRight[48];
    int cnt;

    Char8ToBit64(cipherBlock, cipherBits);
    /* 初始置换（IP置换） */
    DES_IP_Transform(cipherBits);

    /* 16轮迭代 */
    for (cnt = 15; cnt >= 0; cnt--) {
        memcpy(copyRight, cipherBits + 32, 32);
        /* 将右半部分进行扩展置换，从32位扩展到48位 */
        DES_E_Transform(copyRight);
        /* 将右半部分与子密钥进行异或操作 */
        DES_XOR(copyRight, subKeys[cnt], 48);
        /* 异或结果进入S盒，输出32位结果 */
        DES_SBOX(copyRight);
        /* P置换 */
        DES_P_Transform(copyRight);
        /* 将明文左半部分与右半部分进行异或 */
        DES_XOR(cipherBits, copyRight, 32);
        if (cnt != 0) {
            /* 最终完成左右部的交换 */
            DES_Swap(cipherBits, cipherBits + 32);
        }
    }
    /* 逆初始置换（IP^1置换） */
    DES_IP_1_Transform(cipherBits);
    Bit64ToChar8(cipherBits, plainBlock);
}

#define DEFAULT_DES_KEY     ("3uB#*tTy")

int DES__encrypt(const char* input, size_t cb, const char * key, char* output) {
    DES_ElemType keyBlock[8], bKey[64];
    DES_ElemType subKeys[16][48];
    size_t offset;
    size_t length;

    if (!input || 0 == cb || !output || 0 != (cb % 8)) {
        return -1;
    }

    if (!key) {
        memcpy(keyBlock, DEFAULT_DES_KEY, sizeof ( void *));
    }

    /* 将密钥转换为二进制流 */
    Char8ToBit64(keyBlock, bKey);
    /* 生成子密钥 */
    DES_MakeSubKeys(bKey, subKeys);
    /* 8字节对齐分批加密 */
    length = cb;
    offset = 0;
    while (length >= 8) {
        DES_EncryptBlock(&input[offset], subKeys, &output[offset]);
        offset += 8;
        length -= 8;
    }

    return (int)( cb - length);
}

int DES__decrypt(const char* input, size_t cb, const char key[8], char* output) {
    DES_ElemType keyBlock[8], bKey[64];
    DES_ElemType subKeys[16][48];
    size_t offset;
    size_t length;

    if (!input || 0 == cb || !output || 0 != (cb % 8)) {
        return -1;
    }

    if (!key) {
        memcpy(keyBlock, DEFAULT_DES_KEY, sizeof ( void *));
    }

    /* 将密钥转换为二进制流 */
    Char8ToBit64(keyBlock, bKey);
    /* 生成子密钥 */
    DES_MakeSubKeys(bKey, subKeys);
    /* 8字节对齐分批加密 */
    length = cb;
    offset = 0;
    while (length >= 8) {
        DES_DecryptBlock(&input[offset], subKeys, &output[offset]);
        offset += 8;
        length -= 8;
    }

    return (int)( cb - length);
}