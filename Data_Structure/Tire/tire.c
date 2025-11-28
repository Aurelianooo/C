#include "tire.h"

struct TreeNode *initTreeNode(struct TireTree *tree, void *_data, size_t _size, const bool _end)
{
    struct TreeNode *node = (struct TreeNode *)malloc(sizeof(struct TreeNode));
    node->child = (struct TreeNode **)malloc(sizeof(struct TreeNode *) * (tree->childCnt));
    memset(node->child, 0, sizeof(struct TreeNode *) * (tree->childCnt));
    node->data = _data;
    node->size = _size;
    node->end = _end;
    tree->nodeCnt++;
    return node;
}

struct TireTree *initTireTree(const int _childCnt)
{
    struct TireTree *tree = (struct TireTree *)malloc(sizeof(TireTree));
    tree->childCnt = _childCnt;
    tree->wordCnt = 0;
    tree->nodeCnt = 0;
    tree->root = initTreeNode(tree, NULL, 0, false);
    return tree;
}

static void destroyTreeNode(struct TireTree *tree, struct TreeNode *node)
{
    int i;
    if (!node)
        return;
    for (i = 0; i < tree->childCnt; i++)
        if (node->child[i])
            destroyTreeNode(tree, node->child[i]);
    free(node->child);
    free(node->data);
    free(node);
}

void destroyTireTree(struct TireTree *tree)
{
    destroyTreeNode(tree, tree->root);
    free(tree);
}

void insertToTireTree(struct TireTree *tree, const char *s, void *_data, size_t _size)
{
    if (*s == '\0')
        return;
    const char *p = s;
    struct TreeNode *node = tree->root;
    while (*p != '\0')
    {
        int c = *p - 'a';
        if (c < 0 || c >= tree->childCnt)
            return;
        if (!node->child[c])
            node->child[c] = initTreeNode(tree, NULL, 0, false);
        p++;
        node = node->child[c];
    }
    if (node->end)
        return;
    node->end = true;
    node->size = _size;
    node->data = malloc(_size);
    memcpy(node->data, _data, _size);
    tree->wordCnt++;
}

void travelTreeNode(struct TireTree *tree, struct TreeNode *node, void (*callback)(struct TreeNode *, void *), void *arg)
{
    if (!node)
        return;
    if (node->end)
        callback(node, arg);
    int i;
    for (i = 0; i < tree->childCnt; i++)
        if (node->child[i])
            travelTreeNode(tree, node->child[i], callback, arg);
}

void travelTireTree(struct TireTree *tree, void (*callback)(struct TreeNode *, void *), void *arg)
{
    travelTreeNode(tree, tree->root, callback, arg);
}

struct TreeNode *findFromTireTree(struct TireTree *tree, const char *s, int *depth)
{
    const char *p = s;
    struct TreeNode *node = tree->root;
    while (*p != '\0')
    {
        int c = *p - 'a';
        if (c < 0 || c >= tree->childCnt)
            return NULL;
        if (node->child[c])
        {
            p++;
            node = node->child[c];
        }
        else
            return NULL;
    }
    if (node->end)
    {
        if (depth)
            *depth = p - s;
        return node;
    }
    else
        return NULL;
}

void removeFromTireTree(struct TireTree *tree, const char *s)
{
    struct TreeNode *node = findFromTireTree(tree, s, NULL);
    if (!node)
        return;
    node->size = 0;
    free(node->data);
    node->data = NULL;
    node->end = false;
    tree->wordCnt--;
}
