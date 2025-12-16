#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int cmp(const void *a, const void * b)
{
    int* c = *(int **)a;
    int* d = *(int **)b;
    return c[0] - d[0];//如果是第二个元素排序就是c[1]与d[1]了
}
int main()
{
    int i, fi = 6;
    int** a = (int **)malloc(sizeof(int *) * 2);
    for(i = 0; i < 2; i++)
        a[i] = (int*)malloc(sizeof(int) * 3);
    for(i = 0; i < 2; i++)
    {
        a[i][0] = fi--;
        a[i][1] = fi--;
        a[i][2] = fi--;
    }
    printf("original:\n");
    for (i = 0; i < 2; i++)
        printf("%d %d %d\n",a[i][0],a[i][1],a[i][2]);

    printf("sorted:\n");
    qsort(a,2, sizeof(int*), cmp);
    for (i = 0; i < 2; i++)
        printf("%d %d %d\n",a[i][0],a[i][1],a[i][2]);
}

