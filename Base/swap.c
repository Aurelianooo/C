#include<stdio.h>

void swap1(int *a, int *b){
	*a = *a + *b;
	*b = *a - *b;
	*a = *a - *b;
}

//       7 = 0111
//      11 = 1011  ^ 为异或，即同为 0，异为 1
//  7 ^ 11 = 1100 = 12
// 12 ^ 11 = 0111 = 7
// 12 ^  7 = 1011 = 11

void swap2(int *a, int *b){
	*a = *a ^ *b;
	*b = *a ^ *b;
	*a = *a ^ *b;
}

int main(void){
	int a = 7;
	int b = 11;
	swap2(&a, &b);
	printf("%d, %d\n", a, b);

	return 0;
}


