#include<stdio.h>

// 假设你在投票选人 如果你和候选人（利益）相同，你就会给他投一票（count+1），如果不同，你就会踩他一下（count-1）当候选人票数为0（count=0）时，就换一个候选人，但因为和你利益一样的人占比超过了一半 不论换多少次 ，最后留下来的都一定是个和你（利益）相同的人。

int majorityElement(int* nums, int numsSize) {
    int c=0,res,i;
    for(i=0;i<numsSize;i++){
    	if(c==0){
    		res=nums[i];
    	}
    	if(nums[i]==res){
    		c+=1;
    	}else{
    		c-=1;
    	}
    }
    return res;
}

int main(void){
	int nums[]={2,2,1,1,1,2,2};
	int res;
	res = majorityElement(nums, 7);
	printf("%d\n", res);

	return 0;
}
