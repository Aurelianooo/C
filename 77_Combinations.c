#include<stdio.h>
#include<stdlib.h>

// 用realloc会超出内存限制，直接为 *returnColumnSizes malloc一定空间反而可以通过

void recursive(int n, int k, int* returnSize, int** returnColumnSizes, int start, int end, int* tmp, int index, int** ans){
	int i;
	if(index==k){
		// 不能直接将指针tmp赋值给ans[*returnSize]，因为其内容一直在改变
		int *t=(int*)malloc(k*sizeof(int));
		for(i=0;i<k;i++) t[i]=tmp[i];
		ans[*returnSize]=t;
		//(*returnColumnSizes) = (int *)realloc(*returnColumnSizes, sizeof(int) * ((*returnSize)+1));
		(*returnColumnSizes)[*returnSize]=k;
		(*returnSize)++;
		return;
	}
	for(i=start; i<=end;i++){
		tmp[index]=i;
		recursive(n, k, returnSize, returnColumnSizes, i+1, end+1, tmp, index+1, ans);
	}
}

int** combine(int n, int k, int* returnSize, int** returnColumnSizes) {
    int** ans=(int **)malloc(200001*sizeof(int *));
    int* tmp=(int *)malloc(k * sizeof(int));
    *returnSize=0;
    //*returnColumnSizes=NULL;
    *returnColumnSizes=(int *)malloc(200001 * sizeof(int));
    recursive(n, k, returnSize, returnColumnSizes, 1, n-k+1, tmp, 0, ans);
    
    return ans;
}

int main(void){
    int n=4, k=2, i, j;
    int returnSize, *returnColumnSizes;
    int** ans=combine(n, k, &returnSize, &returnColumnSizes);
    for(i=0;i<returnSize;i++){
    	for(j=0;j<returnColumnSizes[i];j++){
    		printf("%d ", ans[i][j]);
    	}
    	printf("\n");
    }

    return 0;
}
