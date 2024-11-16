#include<stdio.h>
#include<string.h>

int lengthOfLastWord(char* s) {
    int ans=0, i=0;
    while(s[i]!='\0'){
    	i++;
    }
    i--;
    while(s[i]==' '){
    	i--;
    }
    while(i>=0 && s[i]!=' '){
    	ans++;
    	i--;
    }
    
    return ans;
}

int main(void){
    char* s="   fly me   to   the moon  ";
    int ans=lengthOfLastWord(s);
    printf("%d\n", ans);

    return 0;
}
