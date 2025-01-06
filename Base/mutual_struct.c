#include<stdio.h>

// 在前面的struct不能含有后面struct的实例化对象，
// 但是可以含有后者的指针
// 并且不需要前向声明

struct A{
	struct B* b;
};

struct B{
	struct A a;
};

int main(void){
	struct A aa;
	struct B bb;

	return 0;
}
