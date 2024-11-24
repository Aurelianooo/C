#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void recursive(int* nums, int numsSize, int* returnSize, int** returnColumnSizes, int index, int* tmp, int** ans){
	int i;
	if(index==numsSize){
		int* t=(int *)malloc(numsSize*sizeof(int));
		for(i=0;i<numsSize;i++) t[i]=tmp[i];
		ans[(*returnSize)++]=t;
		return;
	}
	for(i=0;i<numsSize;i++){
		int flag=0;
		for(int j=0;j<index;j++){
			if(tmp[j]==nums[i]) flag=1;
		}
		if(flag) continue;
		tmp[index]=nums[i];
		recursive(nums, numsSize, returnSize, returnColumnSizes, index+1, tmp, ans);
	}
}

int** permute(int* nums, int numsSize, int* returnSize, int** returnColumnSizes) {
    int** ans=(int **)malloc(720*sizeof(int *));
    int* tmp=(int*)malloc(6*sizeof(int));
    memset(tmp, 0, 6*sizeof(int));
    *returnSize=0;
    recursive(nums, numsSize, returnSize, returnColumnSizes, 0, tmp, ans);
	*returnColumnSizes=(int*)malloc((*returnSize)*sizeof(int));
    for(int i=0;i<(*returnSize);i++){
    	(*returnColumnSizes)[i]=numsSize;
    }
    
    return ans;
}

int main(void){
    int nums[3]={1,2,3};
    int numsSize=3, returnSize, *returnColumnSizes, i, j;
    int** ans=permute(nums, numsSize, &returnSize, &returnColumnSizes);
    for(i=0;i<returnSize;i++){
    	for(j=0;j<numsSize;j++){
    		printf("%d ", ans[i][j]);
    	}
    	printf("\n");
    }

    return 0;
}
