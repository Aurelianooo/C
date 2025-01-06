#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void recursive(int n, int* returnSize, char** ans, char* tmp, int index, int l, int r){
	if(l==n && r==n){
		int i;
		char* t=(char*)malloc((2*n+1)*sizeof(char));
		for(i=0;i<index;i++) t[i]=tmp[i];
		t[i]='\0';
		ans[(*returnSize)++]=t;
		return;
	}
	if(l<n){
		tmp[index]='(';
		recursive(n, returnSize, ans, tmp, index+1, l+1, r);
	}
	if(l>r){
		tmp[index]=')';
		recursive(n, returnSize, ans, tmp, index+1, l, r+1);
	}
}


char** generateParenthesis(int n, int* returnSize) {
    char** ans=(char**)malloc(1000*sizeof(char*));
    char* tmp=(char*)malloc((2*n+1)*sizeof(char));
    *returnSize=0;
    recursive(n, returnSize, ans, tmp, 0, 0, 0);
    
    return ans;
}

int main(void){
    int n=3, returnSize, i, j;
    char** ans=generateParenthesis(n, &returnSize);
    for(i=0;i<returnSize;i++) puts(ans[i]);

    return 0;
}
