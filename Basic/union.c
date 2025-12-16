#include<stdio.h>

//大端：高字节存储在低地址上，低字节存储在高地址上
//小端：低字节存储在低地址上，高字节存储在高地址上

typedef union {
	short n;
	char m[sizeof(short)];
}U;

int main(void){

	U u;
	u.n = 0x0102;
	// 0x0102 = 258 = 0000 0001 0000 0010
	// 低				高
	// 0000 0010		0000 0001 小端 m[0]=2 m[1]=1
	// 0000 0001		0000 0010 大端 m[0]=1 m[1]=2
	if(sizeof(short)==2){
		printf("%d %d\n",(int)u.m[0],(int)u.m[1]);
	}else{
		printf("size of short is:%d\n",(int)sizeof(short));
	}
	return 0;
}
