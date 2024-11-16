#include<stdio.h>
#include<string.h>

int lengthOfLongestSubstring(char* s) {
    int hash[256]={0};
    int l=0, r=0, ans=0;
    int n=strlen(s);
    if(n==1) return 1;
    while(r<n){
    	if(hash[s[r]]==0){
    		hash[s[r]]++;
    		r++;
            if(r-l>ans) ans=r-l;
    	}else{
    		if(r-l>ans) ans=r-l;
    		while(l<r && hash[s[r]]>0){
    			hash[s[l]]--;
    			l++;
    		}
    	}
    }
    
    return ans;
}

int main(void){
    char* s="abcabcbb";
    int ans=lengthOfLongestSubstring(s);
    printf("%d\n", ans);

    return 0;
}
