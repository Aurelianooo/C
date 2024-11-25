#include<stdio.h>

int singleNumber(int* nums, int numsSize) {
    int ans=0, i;
    for(i=0;i<numsSize;i++)
    	ans ^= nums[i];
    	
    return ans;
}

int main(void){
    int nums[5]={4,1,2,1,2};
    int returnSize=5;
    printf("%d\n", singleNumber(nums, returnSize));

    return 0;
}
