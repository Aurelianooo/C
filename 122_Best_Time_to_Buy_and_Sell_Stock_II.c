#include<stdio.h>
#include<math.h>  // gcc XXX -lm
#include <limits.h>  // INT_MIN表示Int型最小值
#include <stdbool.h>

int dfs(int* prices, int i, bool hold){
	if(i<0){
		if(hold){
			return INT_MIN;
		}else{
			return 0;
		}
	}else{
		if(hold){
			return fmax(dfs(prices, i-1, 1), dfs(prices, i-1, 0)-prices[i]);
		}else{
			return fmax(dfs(prices, i-1, 0), dfs(prices, i-1, 1)+prices[i]);
		}
	}
}

int maxProfit(int* prices, int pricesSize) {
	return dfs(prices, 5, 0);
}

int main(void){
//    int prices[5] = {1,2,3,4,5};
    int prices[6] = {7,1,5,3,6,4};
    int res;
    res = maxProfit(prices, 6);
    printf("%d\n", res);

    return 0;
}
