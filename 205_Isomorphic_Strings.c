#include<stdio.h>
#include<stdbool.h>
#include<string.h>

bool isIsomorphic(char* s, char* t) {
    int hasha[256]={0};
    int hashb[256]={0};
    int a=0, b=0, i;
    for(i=0;s[i]!='\0';i++){
    	if(hasha[s[i]]==0) hasha[s[i]]=++a;
    	if(hashb[t[i]]==0) hashb[t[i]]=++b;
    	if(hasha[s[i]]!=hashb[t[i]]) return false;
    }
    
    return true;
}

int main(void){
    char* s="paper";
    char* t="title";
    if(isIsomorphic(s, t)) printf("true\n");
    else printf("false\n");

    return 0;
}
