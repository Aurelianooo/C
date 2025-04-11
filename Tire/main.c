#include <stdio.h>
#include "tire.h"

int main(void)
{
    struct TireTree *tree = initTireTree(26); // lowercase letters only
    insertToTireTree(tree, "hello", 2);
    insertToTireTree(tree, "helloworld", 3);
    insertToTireTree(tree, "hel", 4);
    insertToTireTree(tree, "worldhello", 5);
    struct TreeNode *res = NULL;
    char *str = "hello";
    res = findFromTireTree(tree, str, NULL);
    if (res)
        printf("Tire tree has %s\n", str);
    removeFromTireTree(tree, str);
    printf("Tire tree has %d words\n", tree->wordCnt);
    res = findFromTireTree(tree, str, NULL);
    if (!res)
        printf("Tire tree has not %s\n", str);
    str = "he";
    res = findFromTireTree(tree, str, NULL);
    if (res)
        printf("Tire tree has %s\n", str);
    else
        printf("Tire tree has not %s\n", str);

    destroyTireTree(tree);
    return 0;
}
