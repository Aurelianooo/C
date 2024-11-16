#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int* productExceptSelf(int* nums, int numsSize, int* returnSize) {
    int* l=(int *)malloc(numsSize * sizeof(int));
    int* r=(int *)malloc(numsSize * sizeof(int));
    int i;
    l[0]=1;
    for(i=1;i<numsSize;i++){
    	l[i]=l[i-1]*nums[i-1];
    }
    r[numsSize-1]=1;
    for(i=numsSize-2;i>-1;i--){
    	r[i]=r[i+1]*nums[i+1];
    }
    for(i=0;i<numsSize;i++){
    	l[i]=l[i]*r[i];
    }
    return l;
}

int main(void){
    int nums[4]={1,2,3,4};
    int returnSize=4, i;
    int* ans=productExceptSelf(nums, returnSize, &returnSize);
    for(i=0;i<returnSize;i++){
    	printf("%d ", ans[i]);
    }
    printf("\n");

    return 0;
}
