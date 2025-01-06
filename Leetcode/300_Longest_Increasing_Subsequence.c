#include<stdio.h>
#include<stdlib.h>

int lengthOfLIS(int* nums, int numsSize) {
    int* ans=(int*)malloc(numsSize*sizeof(int));
    int i,j;
    for(i=0;i<numsSize;i++) ans[i]=1;
    for(i=1;i<numsSize;i++){
    	for(j=i-1;j>=0;j--){
    		if(nums[i]>nums[j] && ans[i]<ans[j]+1)
    			ans[i]=ans[j]+1;
    	}
    }
    int tmp=1;
    for(i=0;i<numsSize;i++){
        if(ans[i]>tmp) tmp=ans[i];
    }
    return tmp;
}

int main(void){
    int nums[9]={1,3,6,7,9,4,10,5,6};
    int numsSize=9;
    printf("%d\n", lengthOfLIS(nums, numsSize));

    return 0;
}
