#include<stdio.h>
#include<string.h>
#include<stdlib.h>

char* reverseWords(char* s) {
    char* ans=(char *)malloc(10000);
    int r=strlen(s)-1;
    int start=0;
    while(s[start]==' '){
    	start++;
    }
    while(r>=0 && s[r]==' '){
    	r--;
    }
    int l=r, i=0, j;
    while(l>=0){
    	while(l>=0 && s[l]!=' '){
    		l--;
    	}
    	for(j=1;j<=r-l;j++){
    		ans[i]=s[l+j];
    		i++;
    	}
    	if(l!=-1){
    		if(l!=start-1){
    			ans[i]=' ';
    			i++;
    		}
    		r=l;
    		while(r>=0 && s[r]==' ') r--;
    		l=r;
    	}
    }
    ans[i]='\0';
    
    return ans;
}

int main(void){
    char* s="  a good   example ";
    char* ans=reverseWords(s);
    puts(ans);
    free(ans);
    ans=NULL;

    return 0;
}
