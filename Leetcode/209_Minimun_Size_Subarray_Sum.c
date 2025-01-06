#include<stdio.h>

int minSubArrayLen(int target, int* nums, int numsSize) {
    int ans=0, tmp=0;
    int l=-1,r=-1;
    while(r<numsSize){
    	if(tmp<target){
	    	r++;
	    	if(r>=numsSize) break;
    		tmp+=nums[r];
    	}else{
    		while(l<r && tmp>=target){
    			l++;
    			tmp-=nums[l];
    		}
    		if(ans==0 || ans>r-l+1) ans=r-l+1;
    	}
    }
    
    return ans;
}

int main(void){
    int nums[6]={2,3,1,2,4,3};
    int target=7;
    //int nums[3]={1,4,4};
    int ans=minSubArrayLen(target,nums,6);
    printf("%d\n", ans);

    return 0;
}
