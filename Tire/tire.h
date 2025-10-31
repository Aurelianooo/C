#ifndef TIRE_TREE_H
#define TIRE_TREE_H

#include <stdbool.h>

typedef struct TreeNode
{
    struct TreeNode **child;
    size_t size;
    void* data;
    bool end;
} TreeNode;

typedef struct TireTree
{
    struct TreeNode *root;
    int childCnt;
    int wordCnt;
    int nodeCnt;
} TireTree;

struct TreeNode *initTreeNode(struct TireTree *tree, void* _data, size_t _size, const bool _end);
struct TireTree *initTireTree(const int _childCnt);
void destroyTireTree(struct TireTree *tree);
void insertToTireTree(struct TireTree *tree, const char *s, void* _data, size_t _size);
struct TreeNode *findFromTireTree(struct TireTree *tree, const char *s, int *depth);
void removeFromTireTree(struct TireTree *tree, const char *s);

#endif
