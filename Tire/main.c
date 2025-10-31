#include <stdio.h>
#include "tire.h"

int main(void)
{
    struct TireTree *tree = initTireTree(26); // lowercase letters only
    int data = 2;
    insertToTireTree(tree, "hello", (void *)&data, sizeof(int));
    data = 3;
    insertToTireTree(tree, "helloworld", (void *)&data, sizeof(int));
    data = 4;
    insertToTireTree(tree, "hel", (void *)&data, sizeof(int));
    data = 5;
    insertToTireTree(tree, "worldhello", (void *)&data, sizeof(int));
    struct TreeNode *res = NULL;
    char *str = "hello";
    res = findFromTireTree(tree, str, NULL);
    if (res)
    {
        printf("Tire tree has %s\n", str);
        printf("Node data is %d\n", *(int *)res->data);
    }
    removeFromTireTree(tree, str);
    printf("Tire tree has %d words\n", tree->wordCnt);
    res = findFromTireTree(tree, str, NULL);
    if (!res)
        printf("Tire tree has not %s\n", str);
    str = "hel";
    res = findFromTireTree(tree, str, NULL);
    if (res)
        printf("Tire tree has %s\n", str);
    else
        printf("Tire tree has not %s\n", str);

    printf("Tree root childcnt=%d, wordcnt=%d, nodecnt=%d\n", tree->childCnt, tree->wordCnt, tree->nodeCnt);
    destroyTireTree(tree);
    return 0;
}
