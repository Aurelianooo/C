#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "SegTree.h"

Node *createNode(int left, int right)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->left = left;
    newNode->right = right;
    newNode->value = 0;
    newNode->leftChild = newNode->rightChild = NULL;
    return newNode;
}

Node *buildTree(int arr[], int left, int right, MergeFunc merge)
{
    Node *root = createNode(left, right);
    if (left == right)
    {
        root->value = arr[left];
    }
    else
    {
        int mid = (left + right) / 2;
        root->leftChild = buildTree(arr, left, mid, merge);
        root->rightChild = buildTree(arr, mid + 1, right, merge);
        root->value = merge(root->leftChild->value, root->rightChild->value);
    }
    return root;
}

// 查询指定区间的聚合值
int query(Node *root, int queryLeft, int queryRight, MergeFunc merge, IdentityFunc identity)
{
    if (queryLeft <= root->left && queryRight >= root->right)
    {
        return root->value;
    }
    if (queryLeft > root->right || queryRight < root->left)
    {
        return identity();
    }

    int leftVal = query(root->leftChild, queryLeft, queryRight, merge, identity);
    int rightVal = query(root->rightChild, queryLeft, queryRight, merge, identity);
    return merge(leftVal, rightVal);
}

void freeTree(Node *root)
{
    if (root == NULL)
    {
        return;
    }
    freeTree(root->leftChild);
    freeTree(root->rightChild);
    free(root);
}

void update(Node *root, int index, int newValue, MergeFunc merge)
{
    if (root->left == root->right)
    {
        root->value = newValue;
        return;
    }
    int mid = (root->left + root->right) / 2;
    if (index <= mid)
    {
        update(root->leftChild, index, newValue, merge);
    }
    else
    {
        update(root->rightChild, index, newValue, merge);
    }
    root->value = merge(root->leftChild->value, root->rightChild->value);
}

// ---- 具体聚合算子 ----
int merge_sum(int a, int b) { return a + b; }
int ident_sum(void) { return 0; }

int merge_max(int a, int b) { return a > b ? a : b; }
int ident_max(void) { return INT_MIN; }

int merge_min(int a, int b) { return a < b ? a : b; }
int ident_min(void) { return INT_MAX; }

int main()
{
    int arr[] = {1, 3, 5, 7, 9, 11};
    int n = sizeof(arr) / sizeof(arr[0]);

    int queryLeft = 1;
    int queryRight = 3;

    // 区间和
    Node *sumTree = buildTree(arr, 0, n - 1, merge_sum);
    int sum = query(sumTree, queryLeft, queryRight, merge_sum, ident_sum);
    printf("Sum  of segment [%d, %d]: %d\n", queryLeft, queryRight, sum);

    int index = 2;
    int newValue = 6;
    update(sumTree, index, newValue, merge_sum);
    sum = query(sumTree, queryLeft, queryRight, merge_sum, ident_sum);
    printf("Sum  of updated segment [%d, %d]: %d\n", queryLeft, queryRight, sum);
    freeTree(sumTree);

    // 区间最大值
    Node *maxTree = buildTree(arr, 0, n - 1, merge_max);
    int mx = query(maxTree, queryLeft, queryRight, merge_max, ident_max);
    printf("Max  of segment [%d, %d]: %d\n", queryLeft, queryRight, mx);
    freeTree(maxTree);

    // 区间最小值
    Node *minTree = buildTree(arr, 0, n - 1, merge_min);
    int mn = query(minTree, queryLeft, queryRight, merge_min, ident_min);
    printf("Min  of segment [%d, %d]: %d\n", queryLeft, queryRight, mn);
    freeTree(minTree);

    return 0;
}
