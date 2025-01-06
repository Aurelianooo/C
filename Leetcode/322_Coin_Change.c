#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int coinChange(int* coins, int coinsSize, int amount) {
	int* ans=(int*)malloc((amount+1)*sizeof(int));
	memset(ans, -1, (amount+1)*sizeof(int));
	ans[0]=0;
	int i, j;
	for(i=1;i<amount+1;i++){
		for(j=0;j<coinsSize;j++){
			if(i-coins[j]>=0 && ans[i-coins[j]]!=-1){
				if(ans[i]>ans[i-coins[j]]+1 || ans[i]==-1)
					ans[i]=ans[i-coins[j]]+1;
			}
		}
	}
	
	return ans[amount];
}

int main(void){
    int coins[3]={1,2,5};
    int coinsSize=3, amount=11;
    printf("%d\n", coinChange(coins, coinsSize, amount));

    return 0;
}
