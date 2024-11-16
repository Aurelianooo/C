#include<stdio.h>
#include<stdlib.h>
#include<string.h>

char** summaryRanges(int* nums, int numsSize, int* returnSize) {
	char** ans=(char **)malloc(numsSize * sizeof(char *));
	*returnSize=0;
	int i=0,l,r;
	while(i<numsSize){
		l=i;
		i++;
		while(i<numsSize && nums[i]-1==nums[i-1]) i++;
		r=i-1;
		char* tmp=(char *)malloc(25* sizeof(char));
		sprintf(tmp, "%d", nums[l]);
		if(l<r){
			sprintf(tmp+strlen(tmp),"->");
			sprintf(tmp+strlen(tmp),"%d", nums[r]);
		}
		ans[(*returnSize)++]=tmp;
	}
	
	return ans;
}

int main(void){
    int nums[7]={0,2,3,4,6,8,9};
    int returnSize=0, i;
    char** ans=summaryRanges(nums, 7, &returnSize);
    for(i=0;i<returnSize;i++){
    	puts(ans[i]);
    }

    return 0;
}
