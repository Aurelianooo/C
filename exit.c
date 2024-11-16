#include<stdio.h>
#include<stdlib.h>

void func(){
	exit(1);
}

void clean(){
	system("rm -rvf aa");
}

int main(void){
	system("touch aa");
	atexit(clean);
	func();
	return 0;

}
