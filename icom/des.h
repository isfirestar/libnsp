#if !defined (ALOG_DES_H)
#define ALOG_DES_H

#include "compiler.h"

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
        
#endif /* !ALOG_DES_H */