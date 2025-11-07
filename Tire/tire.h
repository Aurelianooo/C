#ifndef TIRE_TREE_H
#define TIRE_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct TreeNode
{
    struct TreeNode **child;
    void *data;
    size_t size;
    bool end;
} TreeNode;

typedef struct TireTree
{
    struct TreeNode *root;
    int childCnt;
    int wordCnt;
    int nodeCnt;
} TireTree;

struct TreeNode *initTreeNode(struct TireTree *tree, void *_data, size_t _size, const bool _end);
struct TireTree *initTireTree(const int _childCnt);
void destroyTireTree(struct TireTree *tree);
void insertToTireTree(struct TireTree *tree, const char *s, void *_data, size_t _size);
void travelTreeNode(struct TireTree *tree, struct TreeNode *node, void (*callback)(struct TreeNode *, void *), void *arg);
void travelTireTree(struct TireTree *tree, void (*callback)(struct TreeNode *, void *), void *arg);
struct TreeNode *findFromTireTree(struct TireTree *tree, const char *s, int *depth);
void removeFromTireTree(struct TireTree *tree, const char *s);

#endif
