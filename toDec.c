#include<stdio.h>
#include<ctype.h>

int main(void){
	int n;
	char ch[64];
	int i=0;
	int res=0;
	printf("2,8,16\n");
	scanf("%d", &n);
	getchar();
	printf("input number:");
	
	while((ch[i]=getchar()) != '\n'){
		if(ch[i]>='A'){
			ch[i] = tolower(ch[i]);
			res = res * n + 10 + ch[i]-'a';
		}else{
			res = res * n + ch[i]-'0';
		}
		i++;
	}
	printf("%d\n", res);
	return 0;
}
