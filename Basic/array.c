#include<stdio.h>

int main(void){
	int a[5]={1,2,3,4,5};
	
	int *p1=a;
	printf("%d\n",3[p1]);
	
	int *p2 = (int *)(&a + 1);
	printf("%d, %d\n", *(a+1), *(p2-1));
	
	int *p3 = a+1;
	printf("%d\n", *p3);
	
	return 0;
	
	// 3[p]==*(3+p)==*(p+3)==p[3]
	// a 是数组名，表示数组中第一个元素的地址
	// &a 表示整个数组在内存中的起始地址
}
