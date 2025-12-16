#include<stdio.h>
#include<string.h>
#include<stdlib.h>

void getmemory(char **p){
	*p=(char *)malloc(10);
	strcpy(*p,"hello");
}

char* getmem(){
	char*p=(char*)malloc(10);
	strcpy(p,"hello");
	return p;
}

void get(char* p){
	p=(char*)malloc(10);
	strcpy(p,"hello");
}

void change(char* p){
	if(NULL==p) printf("NULL\n");
	strcpy(p,"world");
}

int main(void){
	//char *s = NULL;
	//getmemory(&s);
	
	char* s=getmem();
	
	printf("%s\n",s);
	change(s);
	printf("%s\n",s);
	
	free(s);
	return 0;
}
