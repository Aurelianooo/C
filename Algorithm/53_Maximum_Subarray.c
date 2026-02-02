#include<stdio.h>

int maxSubArray(int* nums, int numsSize) {
	int ans=nums[0], i;
    int tmp=ans;
    for(i=1;i<numsSize;i++){
        if(tmp<=0) tmp=nums[i];
    	else tmp+=nums[i];
    	if(tmp>ans) ans=tmp;
    }
    
    return ans;
}


int main(void){
    int nums[9]={-2,1,-3,4,-1,2,1,-5,4};
    int numsSize=9;
    printf("%d\n", maxSubArray(nums, numsSize));

    return 0;
}
