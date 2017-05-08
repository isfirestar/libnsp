#if !defined (ALGO_BASE64_H)
#define ALGO_BASE64_H

#include "compiler.h"

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

#endif