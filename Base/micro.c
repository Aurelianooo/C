//求1+2+3+……+n的和
//不能使用乘除法，不能使用 if while do-while for switch-case
//不能使用三目运算符

#include<stdio.h>

int calculate(int n, int *sum){
	n && calculate(n-1, sum);
	return *sum += n;
}

int main(void){
	int res = 0;
	calculate(10, &res);
	printf("%d\n", res);
	return 0;
}
