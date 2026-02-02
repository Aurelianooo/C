#include<stdio.h>
#include<string.h>

int strStr(char* haystack, char* needle) {
    char *ans = strstr(haystack, needle);
    if(ans==NULL){
    	return -1;
    }else{
    	/*char* p = haystack;
    	int cnt=0;
    	while(p!=ans){
    		p++;
    		cnt++;
    	}
    	return cnt;*/
    	return ans-haystack;
    }
}

int main(void){
    char* h = "mississippi";
    char* n = "issip";
    int ans = strStr(h, n);
    printf("%d\n", ans);

    return 0;
}
