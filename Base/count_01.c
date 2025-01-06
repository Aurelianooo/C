// 给出一个整数，计算其二进制形式中有多少个0,1
// x-1 相当于将二进制数从右向左数，遇到的第一个1置为0，其右边的所有0置为1
// x & (x-1) 将二进制数从右向左数，遇到的第一个1置为0
// x+1 相当于将二进制数从右向左数，遇到的第一个0置为1，其右边的所有1置为0
// x | (x+1) 将二进制数从右向左数，遇到的第一个0置为1
// 当 x+1 超过最大能表示的值时变为0

#include<stdio.h>

int count_1(int x){
	int c = 0;
	while(x){
		c++;
		x = x & (x-1);
	}
	return c;
}

int count_0(int x){
	int c = 0;
	while(x+1){
		c++;
		x = x | (x+1);
	}
	return c;
}

int main(void){
	int x = 5;
	printf("%d\n", count_1(x));
	printf("%d\n", count_0(x));
	return 0;
}
