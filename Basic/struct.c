#include<stdio.h>

/* 
	第一原则，对齐模数：
	默认就是4, 32位机 4, 64位机 8
	
	第二原则：
	按照 对齐模数 与 结构体中最大的数据类型 中
	较小的那个数据大小进行对齐
	
	第三原则：
	结构体中第一个数据放在offset为0的位置
	 *** 其他数据存储的起始位置必须是自身大小的整数倍
	
	#pragma pack(2) 指定对齐模数为2，必须是2的小次幂（a small power of 2）
*/

typedef struct a{
	double a;
	int b;
	char c;
}A;  // 16

struct b{
	int a;
}B;  // 4

struct {
	short a;
	short b;
	short c;
}C;  // 6

struct{
	char b;
	int a;
	short c;
	short d;
}D;  // 12

struct{
	char a;
	short b;
	char c;
	int d;
}E;  // 12

struct{
	char a;
	double b;
	int c;
}F;

int main(void){
	int size;
	size = sizeof(F);
	printf("%d\n",size);
	return 0;
}

