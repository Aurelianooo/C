#include<stdio.h>

int q_pow(int x, int n){
	int ans=1;
	while(n){
		if(n & 1){
			ans*=x;
		}
		x*=x;
		n=n>>1;
	}
	return ans;
}

int main(void){
    printf("%d\n", q_pow(2,10));

    return 0;
}
