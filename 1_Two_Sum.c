#include<stdio.h>
#include "./uthash/src/uthash.h"

struct HashTable{
	int key;
	int val;
	UT_hash_handle hh;
};

int* twoSum(int* nums, int numsSize, int target, int* returnSize) {
	struct HashTable* hash=NULL;
    int* ans=(int *)malloc((*returnSize) * sizeof(int));
    int i;
    for(i=0;i<numsSize;i++){
    	struct HashTable* ret=NULL;
    	int key=target - nums[i];
    	HASH_FIND_INT(hash, &key, ret);
    	if(ret==NULL){
    		struct HashTable* num=(struct HashTable*)malloc(sizeof(struct HashTable));
    		num->key=nums[i];
    		num->val=i;
    		HASH_ADD_INT(hash, key, num);
    	}else{
    		ans[0]=ret->val;
    		ans[1]=i;
    	}
    }

    return ans;
}

int main(void){
    int nums[4]={2,7,11,15};
    int i,returnSize=2;
    int* ans= twoSum(nums, 4, 9, &returnSize);
    for(i=0;i<returnSize;i++){
    	printf("%d ", ans[i]);
    }
    printf("\n");

    return 0;
}
