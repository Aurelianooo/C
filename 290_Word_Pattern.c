#include<stdio.h>
#include<stdbool.h>
#include<string.h>

bool wordPattern(char* pattern, char* s) {
    char hash[26][3001];
    memset(hash, '\0', 26*3001*sizeof(char));
    int i, j=0, k, count=1;
    for(i=0;i<strlen(s);i++){
        if(s[i]==' ') count++;
    }
    if(count!=strlen(pattern)) return false;

    char tmp[3001], p;
    for(i=0;pattern[i]!='\0';i++){
        memset(tmp, '\0', 3001);
    	k=0;
    	while(j<strlen(s) && s[j]!=' '){
    		tmp[k++]=s[j++];
    	}
    	j++;
    	printf("%s\n", tmp);
    	if(hash[pattern[i]-'a'][0]=='\0'){
    		strcpy(hash[pattern[i]-'a'], tmp);
    	}else if(strcmp(hash[pattern[i]-'a'], tmp)!=0){
    		return false;
    	}
    }
    
    for(i=0;i<26;i++){
        if(hash[i][0]!='\0'){
            for(j=i+1;j<26;j++){
                if(hash[j][0]!='\0'){
                    if(strcmp(hash[i], hash[j])==0) return false;
                }
            }
        }
    }
    
    return true;
}

int main(void){
    char *pattern="abba";
    char *s="dog cat cat pigg";
    if(wordPattern(pattern, s)) printf("true\n");
    else printf("false\n");

    return 0;
}
