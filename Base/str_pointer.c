#include<stdio.h>
#include<string.h>

void change(char *p){
	p[1]='-';
}

int main(void){
	//char str[]="abcdef";  // change OK
	char *str="abcdef";  // if change, segment fault
	//printf("%s\n",str);
	//change(str);
	str+=2;
	printf("%s\n",str);
	printf("%p\n",str);
	
	int a =10;
	int *p = &a;
	printf("%d\n", *p);
	printf("%p\n", p);
	return 0;
}
