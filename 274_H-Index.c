#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int hIndex(int* citations, int citationsSize) {
    int* tmp=(int *)malloc(sizeof(int) * 5001);
    memset(tmp, 0, 5001 * sizeof(int));
    int i,j,res=0;
    for(i=0;i<citationsSize;i++){
    	for(j=1;j<=citations[i];j++){
    		tmp[j]++;
    	}
    }
    for(i=1;i<=citationsSize;i++){
    	if(i<=tmp[i])
    		res = i;
    }
    free(tmp);
    return res;
}

int main(void){
    int nums[5]={3,0,6,1,5},res;
    res = hIndex(nums, 5);
    printf("%d\n", res);

    return 0;
}
