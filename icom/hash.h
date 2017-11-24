/*
 *	����֧�ֵ�һ���Թ�ϣ�㷨��ͳһ����ͷ
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
 * base64_encode ���̶� @incb ���ȵ� @input �������� BASE64 ���ܲ���
 * base64__decode ���̶� @incb ���ȵ� @input �������� BASE64 ���ܲ���
 * 
 * ����:
 * @input ���뻺����
 * @incb ���뻺�����ֽڳ���
 * @utput ���������
 * @outcb ����������ֽڳ��ȼ�¼�� *outcb
 * 
 * ��ע:
 * 1. @input ��������Ч��������ַ
 * 2. @incb ���뱣֤���ڵ���0
 * 3. @output ���ΪNULL, �� @outcb ��������Ч�������� ��������� �� *outcb ����¼���ܺ�Ļ��������ȣ� ������ִ�м��ܲ���
 * 
 * ����:
 * ͨ���ж�
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
DES__encrypt ���̣� ʹ��DES���ڴ����

����:
 * @input ��Ҫ���м��ܵĻ�����
 * @cb ���ܻ���������
 * @key �˸��ֽڵ���Կ
 * @output  ���ܺ�����������, ������������ @input һ��

��ע:
 * @key ����ΪNULL, ���@keyΪNULL, ��ʹ��Ĭ����Կ
 * @cb ����8�ֽڶ���
 */ 
__extern__
int DES__encrypt(const char* input,size_t cb,const char key[8], char* output);
__extern__
int DES__decrypt(const char* input,size_t cb,const char key[8], char* output);

#endif