#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define Max(a,b) (a>b?a:b)

int KnapSack(int n, int* weight, int* value, int capacity, int** V){
	int i,j;
	for(i=1;i<=n;i++){
		for(j=1;j<=capacity;j++){
			if(j<weight[i-1])  // 背包容量比该物品重量小，装不下
				V[i][j]=V[i-1][j];  // 此时价值与前i-1个物品的价值相同
			else  // 可以放入该物品，判断是否装入
				V[i][j]=Max(V[i-1][j],V[i-1][j-weight[i-1]]+value[i-1]);
		}
	}
	return V[n][capacity];
}

void Judge(int capacity, int n, int* weight, int** V){
	int i;
	int* state=(int*)malloc((n+1)*sizeof(int));
	memset(state, 0, n*sizeof(int));
	for(i=n;i>0;i--){  //如果装了就标记，然后减去相应容量 
		if(V[i][capacity]>V[i-1][capacity]){
			state[i]=1;
			capacity=capacity-weight[i-1];
		}
	}
	printf("Chosen items:");
	for(i=1;i<=n;i++)
		if(state[i]) printf("%d ", i);
	printf("\n");
}

int main(void){
    int n=5, i, j;  // 物品数量
    int capacity=10;  // 背包容量
    int weight[5]={2,2,6,5,4};  // 各物品重量
    int value[5]={6,3,5,4,6};  // 各物品价值
    printf("Weight:");
    for(i=0;i<n;i++) printf("%d ",weight[i]);
    printf("\nValue: ");
    for(i=0;i<n;i++) printf("%d ",value[i]);
    printf("\n");
    int** V=(int**)malloc((n+1)*sizeof(int*));
    for(i=0;i<=n;i++){
    	int* tmp=(int*)malloc((capacity+1)*sizeof(int));
    	memset(tmp, 0, n*sizeof(int));
    	V[i]=tmp;
    }
    
    int ans=KnapSack(n, weight, value, capacity, V);
    Judge(capacity, n, weight, V);
    printf("Max value is:%d\n", ans);
    for(i=0;i<=n;i++){
    	for(j=0;j<=capacity;j++)
    		printf("%2d ",V[i][j]);
    	printf("\n");
    }

    return 0;
}
