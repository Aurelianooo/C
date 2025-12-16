#include<stdio.h>

void func1();

int main(void){
	func1();
	printf("%p\n", func1);
	void (*p)()=func1;
	(*p)();
	
	//让程序跳转到绝对地址是0x100000去执行
	//(*(void (*)())0x100000)();
	//先将地址强制类型转换为函数指针，然后调用
	return 0;
}

void func1(){
	printf("This is func1\n");
}
