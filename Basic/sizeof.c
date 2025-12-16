#include<stdio.h>

//sizeof 不是函数，是运算符
// 这里的a++表达式不会进行执行，只计算返回表达式结果的类型大小

int main(void){
	int a = 10;
	printf("%d\n", a);
	printf("%ld\n", sizeof a);
	printf("%ld\n", sizeof(a++));
	printf("%d\n", a);
	return 0;
}
