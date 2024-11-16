#include<stdio.h>
#include<string.h>

int main(int argc, char* args[]){
	if(argc<2){
		printf("Pls enter ur ID:\n");
	}else{
		if(strcmp(args[1],"admin")==0){
			printf("Welcome admin.\n");
		}else if(strcmp(args[1],"teacher")==0){
			printf("Welcome teacher.\n");
		}else if(strcmp(args[1],"student")==0){
			printf("Welcome student.\n");
		}else{
			printf("Welcome visiter.\n");
		}
	}
	return 0;
}
