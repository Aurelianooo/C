#include<stdio.h>
#include<string.h>
#include<stdlib.h>

char* longestCommonPrefix(char** strs, int strsSize) {
    char* ans=(char *)malloc(200);
    int i=0, j,flag;
    char tmp;
    while(i<200){
    	flag=1;
    	tmp=' ';
    	for(j=0;j<strsSize;j++){
    		if(strs[j][i]=='\0'){
    			flag=0;
    			break;
    		}else if(tmp==' '){
    			tmp=strs[j][i];
    		}else if(tmp!=strs[j][i]){
    			flag=2;
    			break;
    		}
    	}
    	if(flag==0 || flag==2){
    		break;
    	}else{
    		ans[i]=tmp;
    		i++;
    	}
    }
    ans[i]='\0';
    
    return ans;
}

int main(void){
    char* strs[200]={"flower","flow","flight"};
    char* p=longestCommonPrefix(strs, 3);
    puts(p);
    free(p);
    p=NULL;

    return 0;
}
