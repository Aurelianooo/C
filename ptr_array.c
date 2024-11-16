#include<stdio.h>

int main(void){
	int a[]={1,2,3,4,5,6};
	int (*p)[6] = &a;
	//定义了一个数组指针，指向一个大小为6的数组,数组中的每个元素都是int类型
	printf("%d\n", (*p+1)[1]);
	printf("%d\n", *p[0]);
	return 0;
}
