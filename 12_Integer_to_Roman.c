#include<stdio.h>
#include<string.h>
#include<stdlib.h>

// 使用malloc函数动态分配，但是一定要注意在主调函数中将其释放，
// 因为malloc动态分配的内存位于堆区，而堆区的内存是要程序员自己释放的。

char* intToRoman(int num) {
    char* ans=(char *)malloc(20);
    int i=0,t;
	t=num/1000;
	if(t>0){
		while(t>0){
			ans[i]='M';
			i++;
			t--;
		}
	}
	num=num%1000;
	t=num/500;
	if(t==1){
		if(num/100==9){
			ans[i]='C';
			i++;
			ans[i]='M';
			i++;
		}else{
			ans[i]='D';
			i++;
			num=num-500;
			t=num/100;
			while(t>0){
				ans[i]='C';
				i++;
				t--;
			}
		}
	}else{
		t=num/100;
		if(t==4){
			ans[i]='C';
			i++;
			ans[i]='D';
			i++;
		}else{
			while(t>0){
				ans[i]='C';
				i++;
				t--;
			}
		}
	}
	num=num%100;
	t=num/50;
	if(t==1){
		if(num/10==9){
			ans[i]='X';
			i++;
			ans[i]='C';
			i++;
		}else{
			ans[i]='L';
			i++;
			num=num-50;
			t=num/10;
			while(t>0){
				ans[i]='X';
				i++;
				t--;
			}
		}
	}else{
		t=num/10;
		if(t==4){
			ans[i]='X';
			i++;
			ans[i]='L';
			i++;
		}else{
			while(t>0){
				ans[i]='X';
				i++;
				t--;
			}
		}
	}
	num=num%10;
	t=num/5;
	if(t==1){
		if(num==9){
			ans[i]='I';
			i++;
			ans[i]='X';
			i++;
		}else{
			ans[i]='V';
			i++;
			num=num-5;
			t=num;
			while(t>0){
				ans[i]='I';
				i++;
				t--;
			}
		}
	}else{
		t=num;
		if(t==4){
			ans[i]='I';
			i++;
			ans[i]='V';
			i++;
		}else{
			while(t>0){
				ans[i]='I';
				i++;
				t--;
			}
		}
	}
	
    
    ans[i]='\0';
    
    return ans;
}

int main(void){
    int num=3749;
    char *p=intToRoman(num);
    puts(p);
    free(p);
    p=NULL;

    return 0;
}
