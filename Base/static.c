#include<stdio.h>
#include"extern.h"


//gcc static.c extern.c extern.h

extern int b;

void test(){
	static int a = 0;
	printf("%d\n", a);
	a++;
}

int main(void){
	test();
	test();
	test();
	test();
	test();
	my_print();
	printf("%d\n",b);
	
	return 0;
}
