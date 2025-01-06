#include<stdio.h>
#include<stdbool.h>

bool canConstruct(char* ransomNote, char* magazine) {
    int hash[26]={0};
    int tmp, i=0;
    while(magazine[i]!='\0'){
    	tmp=magazine[i]-'a';
    	hash[tmp]++;
    	i++;
    }
    i=0;
    while(ransomNote[i]!='\0'){
    	tmp=ransomNote[i]-'a';
    	hash[tmp]--;
    	if(hash[tmp]<0) return false;
    	i++;
    }
    return true;
}

int main(void){
    char *a="aa";
    char *b="aab";
    bool ans=canConstruct(a, b);
    if(ans) printf("true\n");
    else printf("false\n");

    return 0;
}
