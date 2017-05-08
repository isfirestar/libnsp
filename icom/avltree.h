#if !defined (_AVLTREE_HEADER_ANDERSON_20120216)
#define _AVLTREE_HEADER_ANDERSON_20120216

#include "compiler.h"

/* avl search tree, by neo-anderson 2012-05-05 copyright(C) shunwang Co,.Ltd*/

#pragma pack(push, 1)

struct avltree_node_t {
    struct avltree_node_t *lchild, *rchild; /* �ֱ�ָ���������� */
    int height; /* ���ĸ߶� */
};

#pragma pack(pop)

/**
 *	�ṹ����������
 */
typedef struct avltree_node_t TREENODE_T, *PTREENODE, *TREEROOT;

/**
 *	�ڵ����ݶԱ�����
 *
 *	left ���ڶԱȵ���ڵ㣬 ��������ǿ��ת��Ϊstruct avltree_node_t *�����
 *	fight ���ڶԱȵ��ҽڵ㣬 ��������ǿ��ת��Ϊstruct avltree_node_t *�����
 *
 *	��ڵ�����ҽڵ㣬 ����ָ�� 1
 *	�ҽڵ������ڵ㣬 ����ָ�� -1
 *	���ҽڵ���ȣ�     ����ָ�� 0
 */
typedef int( *compare_routine)(const void *left, const void *right);

/**
 *  ���Ϊtree��AVL���������ݡ�
 *
 *  treeָ���������ǰAVL���ĸ���
 *  nodeָ��������������ݵ�avltree_node_t�ڵ㡣
 *  compareָ��ڵ�֮��ıȽϺ��������û����塣
 *
 *  ���ز������ݺ�AVL���ĸ���
 *
 *	���ݽṹ����ʹ�ò�������ָ��ֱ�ӹ����� ��û�н������������ ��Ҫ����������֤�ڵ�ָ���ڵ��� avlremove ֮ǰ����Ч��
 */
__extern__
struct avltree_node_t *avlinsert(struct avltree_node_t *tree, struct avltree_node_t *node,
        int( *compare)(const void *, const void *));
/**
 *  �Ӹ�Ϊtree��AVL��ɾ�����ݡ�
 *
 *  treeָ��ɾ������ǰAVL���ĸ���
 *  nodeָ�������ƥ�����ݵ�avltree_node_t�ڵ㡣
 *  rmnodeΪһ������ָ�룬ɾ���ɹ�ʱ*rmnode��ŵ��Ǳ�ɾ���ڵ�ָ�룬ʧ����ΪNULL��
 *  compareָ��ڵ�֮��ıȽϺ��������û����塣
 *
 *  ����ɾ�����ݺ�AVL���ĸ���
 */
__extern__
struct avltree_node_t *avlremove(struct avltree_node_t *tree, struct avltree_node_t *node,
        struct avltree_node_t **rmnode,
        int( *compare)(const void *, const void *));
/**
 *  �Ӹ�Ϊtree��AVL�����������ݡ�
 *
 *  treeָ��AVL���ĸ���
 *  nodeָ�������ƥ�����ݵ�avltree_node_t�ڵ㡣
 *  compareָ��ڵ�֮��ıȽϺ��������û����塣
 *
 *  ����ƥ�����ݽڵ㣬����NULL��
 */
__extern__
struct avltree_node_t *avlsearch(struct avltree_node_t *tree, struct avltree_node_t *node,
        int( *compare)(const void *, const void *));
/**
 *  �Ӹ�Ϊtree��AVL����������С���ݽڵ㡣
 *
 *  ������С���ݽڵ㣬����NULL����������
 */
__extern__
struct avltree_node_t *avlgetmin(struct avltree_node_t *tree);

/**
 *  �Ӹ�Ϊtree��AVL��������������ݽڵ㡣
 *
 *  ����������ݽڵ㣬����NULL����������
 */
__extern__
struct avltree_node_t *avlgetmax(struct avltree_node_t *tree);


#endif /*_AVLTREE_HEADER_ANDERSON_20120216*/