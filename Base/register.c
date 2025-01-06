#include<stdio.h>
#include<time.h>

// gcc register.c -D REGISTER
// 如果一些变量使用频繁，为提高执行效率，允许将局部变量的值放在CPU寄存器中，
// 不能使用 & 来获取register变量的地址

int main(void){
	long long i=0, sum=0;
#ifdef REGISTER
	register long long value=10;
#else
	long long value=10;
#endif
	time_t start = time(NULL);
	time_t end = start+5;
	
	while(time(NULL)<end){
		sum +=value;
		i++;
	}
	printf("%lld\n", i);
	return 0;
}
