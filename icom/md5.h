#if !defined (ALGO_MD5_H)
#define ALGO_MD5_H

#include "compiler.h"

#pragma pack (push, 1)
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
    uint8_t PADDING[64];
} MD5_CTX;
#pragma pack(pop)

__extern__
void MD5__Init(MD5_CTX *md5ctx);
__extern__
void MD5__Update(MD5_CTX *md5ctx, const uint8_t *input, uint32_t inputLen);
__extern__
void MD5__Final(MD5_CTX *md5ctx, uint8_t digest[16]);

#endif /* ALGO_MD5_H */