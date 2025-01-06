#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

int expandAroundCenter(char* s, int left, int right){
    while(left >= 0 && right < strlen(s) && s[left] == s[right]){
        left--;
        right++;
    }
    return right - left - 1;
}
char* longestPalindrome(char* s) {
    int index = 0, len = 0, i;
    char* ans = malloc(sizeof(char) * 1001);    
    for(i = 0; i < strlen(s); i++){
        int len1 = expandAroundCenter(s, i, i);  // 奇数
        int len2 = expandAroundCenter(s, i, i+1);  // 偶数
        int tmp = fmax(len1, len2);
        if(tmp > len){
            len=tmp;
            index=i-(len-1)/2;
        }
    }
    for(i=0;i<len;i++){
        ans[i] = s[i+index];
    }
    ans[i] = '\0';
    return ans;
}


int main(void){
    char* s="cbbd";
    char* ans=longestPalindrome(s);
    printf("%s\n", ans);

    return 0;
}
