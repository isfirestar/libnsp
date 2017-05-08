#if !defined (ALGO_BASE64_H)
#define ALGO_BASE64_H

#include "compiler.h"

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

#endif