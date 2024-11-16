#include<stdio.h>
#include<stdarg.h>
#include <unistd.h>

void test(int num,...){
	va_list ap;
	va_start(ap,num);
	int i=0;
	for(i=0;i<num;i++){
		printf("%d\n",va_arg(ap,int));
		sleep(1);
	}
	va_end(ap);
}

int main(void){
	test(4,1,2,3,4,3);
	return 0;
}
