#include<stdio.h>
#include<stdbool.h>
#include<string.h>

bool isValid(char* s) {
	char stack[10000]={'\0'};
    int n=strlen(s), i, j=0;
    for(i=0;i<n;i++){
    	switch(s[i]){
    		case '(':
    		case '[':
    		case '{':
    			stack[j]=s[i];
    			j++;
    			break;
    		case ')':
    			if(j<1) return false;
    			j--;
    			if(stack[j]!='(') return false;
    			break;
    		case ']':
    			if(j<1) return false;
    			j--;
    			if(stack[j]!='[') return false;
    			break;
    		case '}':
    			if(j<1) return false;
    			j--;
    			if(stack[j]!='{') return false;
    			break;

    	}
    }
    if(j) return false;
    else return true;
}

int main(void){
    char *s="()[]{}";
    bool ans=isValid(s);
    if(ans) printf("true\n");
    else printf("false\n");

    return 0;
}
