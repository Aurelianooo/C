#include<stdio.h>
#include<stdbool.h>

bool isSubsequence(char *s, char*t){
	while(*s!='\0' && *t!='\0'){
		if(*s == *t){
			s++;
			t++;
		}else{
			t++;
		}
	}
	if(*s=='\0'){
		return true;
	}else{
		return false;
	}
}

int main(void){
    char* t="ahbgdc";
    char* s="abc";
    bool ans=isSubsequence(s, t);
    if(ans){
    	printf("true\n");
    }else{
    	printf("false\n");
    }

    return 0;
}
