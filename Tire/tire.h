
#ifndef TIRE_TREE_H
#define TIRE_TREE_H

#include <stdbool.h>

typedef struct TreeNode
{
    struct TreeNode **child;
    int val;
    bool end;
} TreeNode;

typedef struct TireTree
{
    struct TreeNode *root;
    int childCnt;
    int wordCnt;
    int nodeCnt;
} TireTree;

struct TreeNode *initTreeNode(struct TireTree *tree, const int _val, const bool _end);
struct TireTree *initTireTree(const int _childCnt);
void destroyTireTree(struct TireTree *tree);
void insertToTireTree(struct TireTree *tree, const char *s, const int val);
struct TreeNode *findFromTireTree(struct TireTree *tree, const char *s, int *depth);
void removeFromTireTree(struct TireTree *tree, const char *s);

#endif
