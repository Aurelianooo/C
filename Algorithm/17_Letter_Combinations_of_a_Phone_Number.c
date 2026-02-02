#include<stdio.h>
#include<string.h>
#include<stdlib.h>

static char g_ch[10][4]={"","","abc","def","ghi","jkl","mno","pqrs","tuv","wxyz"};
static int g_total[10]={0,0,3,3,3,3,3,4,3,4};

// tmp暂存每一个解，深度优先递归
// 这里index代表当前递归深度，当index等于要求的字母长度时表明tmp可以作为一个解
// 题目要求最大深度为4，因此tmp空间最大只需要设定为4即可

/*void recursive(char* digits, int* tmp, int index, int depth, char** ans, int* returnSize){
	if(index==depth){
		ans[*returnSize]=(char*)malloc((depth+1)*sizeof(char));
		int i;
		for(i=0;i<depth;i++)
			ans[*returnSize][i]=g_ch[digits[i]-'0'][tmp[i]];
		ans[(*returnSize)++][i]='\0';
		return;
	}
	for(tmp[index]=0;tmp[index]<g_total[digits[index]-'0'];tmp[index]++){
		recursive(digits, tmp, index+1, depth, ans, returnSize);
	}
}*/

void recursive(char* digits, int* tmp, int index, int depth, char** ans, int* returnSize){
	int i;
	if(index==depth){
		ans[*returnSize]=(char*)malloc((depth+1)*sizeof(char));
		for(i=0;i<depth;i++)
			ans[*returnSize][i]=g_ch[digits[i]-'0'][tmp[i]];
		ans[(*returnSize)++][i]='\0';
		return;
	}
	
	for(i=0;i<g_total[digits[index]-'0'];i++){
		tmp[index]=i;
		recursive(digits, tmp, index+1, depth, ans, returnSize);
	}
}

char** letterCombinations(char* digits, int* returnSize) {
    char** ans=(char**)malloc(256*sizeof(char *));
    int depth=strlen(digits);
    int tmp[4]={0};
    *returnSize=0;
    if(!depth) return NULL;
    recursive(digits, tmp, 0, depth, ans, returnSize);
    
    return ans;
}

int main(void){
    char* digits="23";
    int returnSize, i;
    char** ans=letterCombinations(digits, &returnSize);
    for(i=0;i<returnSize;i++){
    	puts(ans[i]);
    }

    return 0;
}
