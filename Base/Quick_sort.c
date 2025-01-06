#include <stdio.h>
#include <stdlib.h>

// 定义一个包含五个整数的数组
int values[] = {88, 56, 100, 2, 25};

// 比较函数，用于比较两个整数
/*如果compar返回值小于0（< 0），那么a所指向元素会被排在b所指向元素的前面
  如果compar返回值等于0（= 0），那么a所指向元素与b所指向元素的顺序不确定
  如果compar返回值大于0（> 0），那么a所指向元素会被排在b所指向元素的后面。*/
int cmpfunc(const void *a, const void *b)
{
   return ((*(int *)a > *(int *)b)-(*(int *)a < *(int *)b));
}

int main()
{
   int n;

   // 输出排序之前的数组内容
   printf("排序之前的列表：\n");
   for (n = 0; n < 5; n++)
   {
      printf("%d ", values[n]);
   }

   // 使用 qsort 函数对数组进行排序
   qsort(values, 5, sizeof(int), cmpfunc);

   // 输出排序之后的数组内容
   printf("\n排序之后的列表：\n");
   for (n = 0; n < 5; n++)
   {
      printf("%d ", values[n]);
   }

   return 0;
}
