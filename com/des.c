#include <string.h>
#include <stdio.h>


/*-----------------------------------------------------------------------DES算法--------------------------------------------*/
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

    return ( cb - length);
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

    return ( cb - length);
}