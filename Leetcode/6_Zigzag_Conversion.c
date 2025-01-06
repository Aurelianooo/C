#include<stdio.h>
#include<string.h>
#include <stdlib.h>

// 题目说是Z型排布，实际上是N型

char* convert(char* s, int numRows) {
    int n=strlen(s);
    char *p=(char *)malloc(n+1);
    p[n]='\0';
    int i,j,k=2*numRows-2, q=0;
    for(i=0;i<numRows;i++){
    	for(j=0;j<n;j++){
    		if(j%k==i || j%k==k-i){
    			p[q++]=s[j];
    		}
    	}
    }
    return p;
}

int main(void){
    char *s = "PAYPALISHIRING";
    char *p = convert(s, 3);
    puts(p);
    free(p);
    p=NULL;

    return 0;
}
