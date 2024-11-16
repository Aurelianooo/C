#include<stdio.h>

void rotate(int* nums, int numsSize, int k){
	k=k%numsSize;
	int tmp[k],i;
	for(i=numsSize-k;i<numsSize;i++){
		tmp[i+k-numsSize]=nums[i];
	}
	for(i=numsSize-1;i>k-1;i--){
		nums[i]=nums[i-k];
	}
	for(i=0;i<k;i++){
		nums[i]=tmp[i];
	}
}

int main(void){
	int nums[]={1,2,3,4,5,6,7},i;
	
	rotate(nums, 7, 3);
	
	for(i=0;i<7;i++){
		printf("%d ",nums[i]);
	}
	printf("\n");

    return 0;
}
