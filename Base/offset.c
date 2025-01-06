#include<stdio.h>
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#pragma pack(2)
//64位linux默认8字节对齐
//可以用以上方法修改struct对齐方式（2）
//宏定义函数 offsetof 用于计算 TYPE 类型中某成员相对于其起始位置的偏移量


typedef struct s{
	union{
		int a;
		char str[10];
	};
	struct s* next;
}S;

int main(void){
	printf("%ld\n", offsetof(S, next));
	return 0;
}
