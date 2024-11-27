#include<stdio.h>

// 第K大的数字在数组中对应的位置为n-k(按照升序排序的话)
// 虽然会超时，但是是正常的快排

int quick_sort(int* nums, int numsSize, int k, int l, int r){
	if(l==r) return nums[l];
	int i=l, j=r;
	int tmp=nums[i];
	
	while(i<j){
		while(i<j && nums[j]>=tmp) j--;
		nums[i]=nums[j];
		while(i<j && nums[i]<=tmp) i++;
		nums[j]=nums[i];
	}
	nums[i]=tmp;
	if(i>numsSize-k) return quick_sort(nums, numsSize, k, l, i-1);
	else if(i<numsSize-k) return quick_sort(nums, numsSize, k, i+1, r);
	else return nums[i];
}

int findKthLargest(int* nums, int numsSize, int k) {
    return quick_sort(nums, numsSize, k, 0, numsSize-1);
}

int main(void){
    int nums[9]={3,2,3,1,2,4,5,5,6};
    int numsSize=9, k=4;
    printf("%d\n", findKthLargest(nums, numsSize, k));

    return 0;
}
