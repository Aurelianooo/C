#include<stdio.h>
#include<string.h>
#include<stdlib.h>

// strtok操作的字符串必须是字符数组而非常量字符串，否则报错段错误
// strtok会自动跳过空字符串
// 这里sprintf可以用strcat代替，实际上strcat算是sprintf的下位替代

char* simplifyPath(char* path) {
    char* stack[100];
    int top=0, i;
    char* tmp;
    for(tmp=strtok(path, "/"); tmp; tmp=strtok(NULL,"/")){
    	if(strcmp(tmp,".")==0){
    		continue;
    	}else if(strcmp(tmp,"..")==0){
    		top--;
    		if(top<0) top=0;
    	}else{
    		stack[top++]=tmp;
    	}
    }
    if(!top) return "/";
    char* ans=(char *)malloc(1000);
    ans[0]='\0';
    for(i=0;i<top;i++){
    	// sprintf(ans+strlen(ans), "/");
    	// sprintf(ans+strlen(ans), "%s", stack[i]);
    	strcat(ans, "/");
    	strcat(ans, stack[i]);
    }
    return ans;
}

int main(void){
	char path[]="/.../a/../b/c/../d/./";
	char* ans = simplifyPath(path);
	puts(ans);

    return 0;
}
