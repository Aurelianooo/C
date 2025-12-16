#include<stdio.h>

int main(void){
	const float ESP=0.000001f;
	float x = 10.0f;
	if(x>=(10-ESP) && x<=(10+ESP)){
		printf("yes\n");
	}else{
		printf("no\n");
	}
	return 0;
}
