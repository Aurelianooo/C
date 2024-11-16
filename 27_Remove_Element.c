#include<stdio.h>

int removeElement(int* nums, int numsSize, int val) {
	int l=0, r=0;
	while(r<numsSize){
		if(nums[r]!=val){
			nums[l]=nums[r];
			l++;
		}
		r++;
	}
	return l;
}

int main(void){
	int nums[8]	= {0,1,2,2,3,0,4,2};
	int val = 2;
	int res = 0,i;
	res = removeElement(nums, 8, 2);
	for(i=0;i<res;i++){
		printf("%d ",nums[i]);
	}
	printf("\n");
	
	return 0;
}

