#include<stdio.h>

int canCompleteCircuit(int* gas, int gasSize, int* cost, int costSize) {
    int ans=-1, i=0, j, tmp;
    while(i<gasSize){
    	tmp = 0;
    	for(j=i;j<i+gasSize;j++){
    		tmp+=gas[j%gasSize];
    		tmp-=cost[j%gasSize];
    		if(tmp<0)
    			break;
    	}
    	if(j-i==gasSize){
    		ans=i;
    		break;
    	}else{
            i=j+1;
        }
    }
    
    return ans;
}

int main(void){
	//int gas[5]={1,2,3,4,5};
	//int cost[5]={3,4,5,1,2};
	int gas[3]={2,3,4};
	int cost[3]={3,4,3};
	int ans;
	ans = canCompleteCircuit(gas, 3, cost, 3);
	printf("%d\n", ans);

    return 0;
}
