#if !defined (ALOG_DES_H)
#define ALOG_DES_H

#include "compiler.h"

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
        
#endif /* !ALOG_DES_H */