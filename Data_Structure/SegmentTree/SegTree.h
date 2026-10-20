#ifndef SEG_TREE_H
#define SEG_TREE_H

typedef struct Node
{
    int left, right;
    int value;
    struct Node *leftChild, *rightChild;
} Node;

// 聚合算子：把两个子区间的值合并成一个区间的值
typedef int (*MergeFunc)(int, int);

// 聚合算子对应的"不相交时的单位元/哨兵值"
typedef int (*IdentityFunc)(void);

// 具体聚合算子
int merge_sum(int a, int b);
int ident_sum(void);

int merge_max(int a, int b);
int ident_max(void);

int merge_min(int a, int b);
int ident_min(void);

// 建树 / 查询 / 单点更新 / 释放
Node *buildTree(int arr[], int left, int right, MergeFunc merge);
int query(Node *root, int queryLeft, int queryRight, MergeFunc merge, IdentityFunc identity);
void update(Node *root, int index, int newValue, MergeFunc merge);
void freeTree(Node *root);

#endif // SEG_TREE_H
