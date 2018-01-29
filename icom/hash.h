/*
 *	整合支持的一致性哈希算法到统一导出头
 *	anderson 2017-11-24
*/

#if !defined HASH_H
#define HASH_H

#include "compiler.h"

/*--------------------------------------------VFN1/VFN1a--------------------------------------------*/
__extern__
uint32_t vfn1_h32( const unsigned char *key, int length );
__extern__
uint64_t vfn1_h64( const unsigned char *key, int length );
__extern__
uint32_t vfn1a_h32( const unsigned char *key, int length );
__extern__
uint64_t vfn1a_h64( const unsigned char *key, int length );

__extern__
uint32_t crc32(uint32_t crc, const unsigned char *string, uint32_t size);

/*
 * base64_encode 例程对 @incb 长度的 @input 缓冲区作 BASE64 加密操作
 * base64__decode 例程对 @incb 长度的 @input 缓冲区作 BASE64 解密操作
 * 
 * 参数:
 * @input 输入缓冲区
 * @incb 输入缓冲区字节长度
 * @utput 输出缓冲区
 * @outcb 输出缓冲区字节长度记录在 *outcb
 * 
 * 备注:
 * 1. @input 必须是有效缓冲区地址
 * 2. @incb 必须保证大于等于0
 * 3. @output 如果为NULL, 则 @outcb 必须是有效缓冲区， 这种情况下 ， *outcb 将记录加密后的缓冲区长度， 但并不执行加密操作
 * 
 * 返回:
 * 通用判定
 */
__extern__
int base64__encode(const char *input, int incb, char *output, int *outcb);
__extern__
int base64__decode(const char *input, int incb, char *output, int *outcb);

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

/*
DES__encrypt 过程， 使用DES对内存加密

参数:
 * @input 需要进行加密的缓冲区
 * @cb 加密缓冲区长度
 * @key 八个字节的密钥
 * @output  加密后的输出缓冲区, 缓冲区长度与 @input 一致

备注:
 * @key 可以为NULL, 如果@key为NULL, 则使用默认密钥
 * @cb 必须8字节对齐
 */ 
__extern__
int DES__encrypt(const char* input,size_t cb,const char key[8], char* output);
__extern__
int DES__decrypt(const char* input,size_t cb,const char key[8], char* output);

#endif