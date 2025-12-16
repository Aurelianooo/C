#include<stdio.h>
#define LIST 0123456789ABCDEF
#define __parse_str(tag) #tag
#define parse_str(tag) __parse_str(tag)

//#x是给x加双引号

int main(void){
	//int LIST[];
	printf("%c\n", parse_str(LIST)[10]);
	return 0;
}
