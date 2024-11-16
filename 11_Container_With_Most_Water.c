#include<stdio.h>

int maxArea(int* height, int heightSize) {
    int ans=0, tmp;
    int l=0,r=heightSize-1;
    while(l<r){
    	if(height[l]>=height[r]){
    		tmp=(r-l)*height[r];
    		if(tmp>ans) ans=tmp;
    		r--;
    	}else{
    		tmp=(r-l)*height[l];
    		if(tmp>ans) ans=tmp;
    		l++;
    	}
    }
    
    return ans;
}

int main(void){
    int nums[9]={1,8,6,2,5,4,8,3,7};
    int res=maxArea(nums, 9);
    printf("%d\n", res);

    return 0;
}
