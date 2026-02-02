#include<stdio.h>

int recursive(int* nums, int numsSize, int target, int l, int r){
	if(l==r) return l;
	int mid=(l+r)/2;
	if(nums[mid]==target) return mid;
	else if(nums[mid]>target) return recursive(nums, numsSize, target, l, mid);
	else return recursive(nums, numsSize, target, mid+1, r);
}

int searchInsert(int* nums, int numsSize, int target) {
    return recursive(nums, numsSize, target, 0, numsSize);
}

int main(void){
    int nums[4]={1,3,5,6};
    int numsSize=4, target=5;
    printf("%d\n", searchInsert(nums, numsSize, target));

    return 0;
}
