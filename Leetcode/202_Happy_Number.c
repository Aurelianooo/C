#include<stdio.h>
#include<stdbool.h>

int cal(int n){
	int res=0;
	int tmp;
	while(n){
		tmp=n%10;
		res+=tmp*tmp;
		n=n/10;
	}
	return res;
}

bool isHappy(int n) {
    int fast=n;
    int slow=n;
    do{
    	slow=cal(slow);
    	fast=cal(fast);
    	fast=cal(fast);
    }while(fast!=slow);
    if(slow!=1) return false;
    else return true;
}

int main(void){
    int n=19;
    bool ans=isHappy(n);
    if(ans) printf("true\n");
    else printf("false\n");

    return 0;
}
