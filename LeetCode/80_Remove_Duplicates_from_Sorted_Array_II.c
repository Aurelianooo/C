#include<stdio.h>

int removeDuplicates(int* nums, int numsSize){
	if(numsSize<=2){
		return numsSize;
	}
	int l=2,r=2;
	while(r<numsSize){
		if(nums[l-2]!=nums[r]){
			nums[l]=nums[r];
			l++;
		}
		r++;
	}
	return l;
}

int main(void){
	int nums[]={0,0,1,1,1,1,2,3,3};
	int res,i;
	res = removeDuplicates(nums, 9);
	printf("%d\n", res);
	for(i=0;i<res;i++){
		printf("%d ", nums[i]);
	}
	printf("\n");

	return 0;
}
