#include<stdio.h>

int main(void){

	int n[3][4]={1,2,3,4,
				 8,7,6,5,
				 9,10,11,12};
				
	int *p[3]={n[2],n[0],n[1]};
	/* 9,10,11,12
	   1, 2, 3, 4,
	   8, 7, 6, 5
	*/
	printf("%d\n", (*(p+2))[1]);
	
	printf("%d\n", p[2][3]);
	
	printf("%d\n", *(p+1)[1]);
	// 最后才执行*
	return 0;
}
