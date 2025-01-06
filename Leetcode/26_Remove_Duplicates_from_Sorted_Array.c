#include<stdio.h>

int removeDuplicates(int* nums, int numsSize) {
	int l=0,r=0;
	while(r<numsSize){
		while(r<numsSize && nums[l]==nums[r]){
			r++;
		}
		if(r<numsSize)
			nums[++l]=nums[r];
	}
	return l+1;
}

int main(void){
	//int nums[10]={0,0,1,1,1,2,2,3,3,4};
	int nums[3]={1,1,2};
	int res, i;
	res = removeDuplicates(nums, 3);
	for(i=0;i<res;i++){
		printf("%d ", nums[i]);
	}
	printf("\n");

	return 0;
}
