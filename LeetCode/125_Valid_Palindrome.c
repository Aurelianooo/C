#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

bool isAlphanumeric(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

bool isPalindrome(char *s)
{
    int l = 0, r = strlen(s) - 1;
    while (l < r)
    {
        while (l < r && !isAlphanumeric(s[l]))
            l++;
        while (l < r && !isAlphanumeric(s[r]))
            r--;
        if (tolower(s[l]) != tolower(s[r]))
            return false;
        l++;
        r--;
    }
    return true;
}

int main(void)
{
    char s[] = "A man, a plan, a canal: Panama";
    bool res = isPalindrome(s);
    printf("%d\n", res);
    return 0;
}
