#include<stdio.h>

int str2int(const char *p){
	int res = 0;
	while(*p>='0' && *p<='9'){
		res = res * 10 + *p - '0';
		p++;
	}
	if(*p != '\0')
		return 0;
	return res;
}

int main(void){

	char *str="12345";
	printf("%d\n", str2int(str));
	
	return 0;
}
