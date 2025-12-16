#include<stdio.h>
#include<setjmp.h>

jmp_buf env;

void func(int arg){
	printf("func: %d\n", arg);
	longjmp(env, ++arg);
	printf("---func---\n");
}

int main(void){
    
    int ret=setjmp(env);
    if(ret==0){
    	printf("ret == 0\n");
    	func(ret);
    }else if(ret==1){
    	printf("ret == 1\n");
    	func(ret);
    }

    return 0;
}
