#include<stdio.h>

// 交换一个8位数的前4bit与后4bit
void convert(unsigned int *);

int main(void){
	unsigned int a = 0x7a;
	convert(&a);
	printf("0x%x\n", a);
	return 0;
}

void convert(unsigned int *p){
	unsigned int tmp;
	tmp = *p & 0x0f;  //获取后4bit
	*p = *p & 0xf0;   //获取前4bit
	*p = (tmp << 4)|(*p >> 4);  // 组合
}
