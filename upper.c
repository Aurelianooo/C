#include<stdio.h>
#include<string.h>
#include<stdlib.h>

//void upperCase(char str[]){
void upperCase(char* str){
	int i;
	for(i=0;i<strlen(str);i++){
		if('a'<=str[i] && str[i]<='z'){
			str[i] -= ('a'-'A');
		}
	}
}

int main(void){
	char str[]="aBcDefg";
	//char *str ="aBcDefg"; //segment fault
	upperCase(str);
	
	printf("%s\n",str);
	
	return 0;
}
