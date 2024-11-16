#include<stdio.h>
#include<stdlib.h> 

int cmpfunc(const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

int longestConsecutive(int* nums, int numsSize) {
    if(!numsSize) return 0;
    int ans=1, i;
    qsort(nums, numsSize, sizeof(int), cmpfunc);
    int tmp=1;
    for(i=1;i<numsSize;i++){
    	if(nums[i]-nums[i-1]==1){
    		tmp++;
    		if(tmp>ans) ans=tmp;
    	}else if(nums[i]==nums[i-1]){
    		continue;
    	}else{
            tmp=1;
        }
    }

    return ans;
}
int main(void){
    int nums[6]={100,4,200,1,3,2};
    int ans=longestConsecutive(nums, 6);
    printf("%d\n", ans);

    return 0;
}
