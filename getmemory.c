#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void getmemory(char **p){
	*p=(char *)malloc(10);
	strcpy(*p,"hello");
}

int main(void){
	char *s = NULL;
	getmemory(&s);
	printf("%s\n",s);
	free(s);
	return 0;
}
