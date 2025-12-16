#include<stdio.h>

int main(void){
	FILE *p;
	long int size =0;
	p = fopen("temp.txt", "r");
	fseek(p, 0, SEEK_END);
	size = ftell(p);
	fclose(p);
	printf("%ld\n", size);
	
	return 0;
}
