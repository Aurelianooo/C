#include<stdio.h>
#include<stdbool.h>

bool canJump(int* nums, int numsSize) {
	int mx=0, i;
	for(i=0;i<numsSize;i++){
		if(i>mx) return false;
		if(i+nums[i]>mx) mx=i+nums[i];
	}
	return true;
}

int main(void){
    int nums[5]={2,3,1,1,4};
    bool res;
    res = canJump(nums, 5);
    if(res){
    	printf("true\n");
    }else{
    	printf("false\n");
    }

    return 0;
}
