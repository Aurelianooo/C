#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int evalRPN(char** tokens, int tokensSize) {
	int i,j,tmp,a,b;
	int* stack=(int *)malloc(10001*sizeof(int));
	j=0;
	for(i=0;i<tokensSize;i++){
		if(strcmp("0", tokens[i])==0){
			stack[j++]=0;
		}else{
			tmp=atoi(tokens[i]);
			if(!tmp){
				if(strcmp("+", tokens[i])==0){
					b=stack[--j];
					a=stack[--j];
					stack[j++]=a+b;
				}else if(strcmp("-", tokens[i])==0){
					b=stack[--j];
					a=stack[--j];
					stack[j++]=a-b;
				}else if(strcmp("*", tokens[i])==0){
					b=stack[--j];
					a=stack[--j];
					stack[j++]=a*b;
				}else if(strcmp("/", tokens[i])==0){
					b=stack[--j];
					a=stack[--j];
					stack[j++]=a/b;
				}
			}else{
				stack[j++]=tmp;
			}
		}
	}
	
    return stack[0];
}

int main(void){
    char tokens[13][5]={"10","6","9","3","+","-11","*","/","*","17","+","5","+"};
    int tokensSize=13;
    char* p[tokensSize], i;
    for(i=0;i<tokensSize;i++){
    	p[i]=tokens[i];
    }
    int ans=evalRPN(p, tokensSize);
    printf("%d\n", ans);
    
    return 0;
}
