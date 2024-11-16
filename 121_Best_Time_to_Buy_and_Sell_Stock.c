#include<stdio.h>

int maxProfit(int* prices, int pricesSize) {
	int i, minn,res=0;
	minn = prices[0];
	for(i=0;i<pricesSize;i++){
		if(prices[i]<minn){
			minn = prices[i];
		}else if(prices[i]-minn>res){
			res = prices[i]-minn;
		}
	}
	return res;
}

int main(void){
    int nums[6]={7,1,5,3,6,4}, res;
    res = maxProfit(nums, 6);
    printf("%d\n", res);

    return 0;
}
