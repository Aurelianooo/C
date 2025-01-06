#include<stdio.h>
#include<stdlib.h>

void recursive(int* candidates, int candidatesSize, int target, int* returnSize, int** returnColumnSizes, int** ans, int start, int sum, int index, int* tmp){
	int i;
	if(sum>target) return;
	else if(sum==target){
		int* t=(int*)malloc(40*sizeof(int));
		for(i=0;i<index;i++) t[i]=tmp[i];
		ans[*returnSize]=t;
		(*returnColumnSizes)[*returnSize]=index;
		(*returnSize)++;
		return;
	}
	for(i=start;i<candidatesSize;i++){
		tmp[index]=candidates[i];
		sum+=candidates[i];
		recursive(candidates, candidatesSize, target, returnSize, returnColumnSizes, ans, start, sum, index+1, tmp);
		// 如果超过，需要将最新加上的减去，并将循环初始加一
		sum-=candidates[i];
		start++;
	}
}

int** combinationSum(int* candidates, int candidatesSize, int target, int* returnSize, int** returnColumnSizes) {
	int** ans=(int**)malloc(1000*sizeof(int *));
	*returnSize=0;
	*returnColumnSizes=(int *)malloc(1000*sizeof(int));
	int* tmp=(int*)malloc(40*sizeof(int));
	recursive(candidates, candidatesSize, target, returnSize, returnColumnSizes, ans, 0, 0, 0, tmp);
	
	if(*returnSize==0) return NULL;
	else return ans;
}

int main(void){
    int candidates[4]={2,3,6,7};
    int candidatesSize=4, target=7, returnSize, *returnColumnSizes, i, j;
    int** ans=combinationSum(candidates, candidatesSize, target, &returnSize, &returnColumnSizes);
    for(i=0;i<returnSize;i++){
    	for(j=0;j<returnColumnSizes[i];j++){
    		printf("%d ", ans[i][j]);
    	}
    	printf("\n");
    }

    return 0;
}
