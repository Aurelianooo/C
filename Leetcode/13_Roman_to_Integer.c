#include<stdio.h>

int romanToInt(char* s) {
	int ans=0, i=0;
	while(s[i] != '\0'){
		switch(s[i]){
			case 'I':
				ans+=1;
				break;
			case 'V':
				ans+=5;
				if(i>0 && s[i-1]=='I'){
					ans-=2;
				}
				break;
			case 'X':
				ans+=10;
				if(i>0 && s[i-1]=='I'){
					ans-=2;
				}
				break;
			case 'L':
				ans+=50;
				if(i>0 && s[i-1]=='X'){
					ans-=20;
				}
				break;
			case 'C':
				ans+=100;
				if(i>0 && s[i-1]=='X'){
					ans-=20;
				}
				break;
			case 'D':
				ans+=500;
				if(i>0 && s[i-1]=='C'){
					ans-=200;
				}
				break;
			case 'M':
				ans+=1000;
				if(i>0 && s[i-1]=='C'){
					ans-=200;
				}
				break;
		}
		i++;
	}
	
	return ans;    
}

int main(void){
    char *s="MCMXCIV";
    int ans=romanToInt(s);
    printf("%d\n", ans);

    return 0;
}
