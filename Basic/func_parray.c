#include<stdio.h>

void call_each(void (**a)(void), int size){
	int n = size;
	while(--n>=0){
		void (*p)(void) = *a++;
		if(p)
			(*p)();
	}
}

void fun1(){
	printf("this is fun1\n");
}

void fun2(){
	printf("this is fun2\n");
}

void fun3(){
	printf("this is fun3\n");
}

int main(void){
	void (*a[3])(void) = {fun1, fun2, fun3};
	call_each(a, 3);

	return 0;
}
