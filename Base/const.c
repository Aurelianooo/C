#include<stdio.h>

int main(void){
	const int a = 10;
	//a=11; 不可修改
	
	char ch1 = 'a';
	char ch2 = 'b';
	
	const char * p1 = &ch1;
	char const * p2 = &ch1;
	// p1与p2相同，指针指向可变，内容不可变
	char * const p3 = &ch1;
	// p3指针指向不可变，内容可变
	//const * char p4 = &ch1; 不能这样定义，报错
	//以*为界限，与const相邻的不可变且const一定要有一个相邻的
	const char * const p4 = &ch1;
	
	return 0;
	
}
