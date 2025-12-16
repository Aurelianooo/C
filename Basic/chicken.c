#include<stdio.h>

void dfs(int a, int b){
	int c = 100-a-b;
	if((a*5 + b*3 + c/3) == 100 && c%3 ==0){
		printf("%d,%d,%d\n", a,b,c);
	}
	if(b==33){
		if(a!=20){
			dfs(a+1,0);
		}
	}else{
		dfs(a,b+1);
	}
	
}

int main(void){
	dfs(0, 0);
	
	return 0;
}

