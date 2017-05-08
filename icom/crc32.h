#if !defined ALGO_CRC32_H
#define ALGO_CRC32_H

#include "compiler.h"

__extern__
uint32_t crc32(uint32_t crc, const unsigned char *string, uint32_t size);

#endif /* !ALGO_CRC32_H */