#include <stdio.h>
#include <stdlib.h>

void func1()
{
    int **a; // 用二级指针动态申请二维数组
    int i, j;
    int m, n;
    printf("请输入行数\n");
    scanf("%d", &m);
    printf("请输入列数\n");
    scanf("%d", &n);
    a = (int **)malloc(sizeof(int *) * m);
    for (i = 0; i < m; i++)
        a[i] = (int *)malloc(sizeof(int) * n);
    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            printf("%p\n", &a[i][j]); // 输出每个元素地址，每行的列与列之间的地址时连续的，行与行之间的地址不连续
        }
    }
    for (i = 0; i < m; i++)
        free(a[i]);

    free(a);
}

void func2()
{
    int i, j;
    // 申请一个3行2列的整型数组
    int(*a)[2] = (int(*)[2])malloc(sizeof(int) * 3 * 2);
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 2; j++)
        {
            printf("%p\n", &a[i][j]); // 输出数组每个元素地址，每个元素的地址是连续的
        }
    }

    free(a);
}

int main()
{
    func1();
    // func2();
    return 0;
}
