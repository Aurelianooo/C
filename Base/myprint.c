#include <stdio.h>
#include <stdarg.h> /* 使用va_list、va_start等必须包含的头文件 */
#include <string.h>
#include <math.h>

/* linux C没有itoa函数,所以要自己写 */
char *itoa(int i, char *str)
{
    int mod, div = fabs(i), index = 0;
    char *start, *end, temp;

    do
    {
        mod = div % 10;
        str[index++] = '0' + mod;
        div = div / 10;
    } while (div != 0);

    if (i < 0)
        str[index++] = '-';

    str[index] = '\0';

    for (start = str, end = str + strlen(str) - 1;
         start < end; start++, end--)
    {
        temp = *start;
        *start = *end;
        *end = temp;
    }

    return str;
}

void myPrint(const char *fmt, ...)
{
    char str[100];
    unsigned int len, i, index;
    int iTemp;
    char *strTemp;
    va_list args;

    va_start(args, fmt);
    len = strlen(fmt);
    for (i = 0, index = 0; i < len; i++)
    {
        if (fmt[i] != '%') /* 非格式化参数 */
        {
            str[index++] = fmt[i];
        }
        else /* 格式化参数 */
        {
            switch (fmt[i + 1])
            {
            case 'd': /* 整型 */
            case 'D':
                iTemp = va_arg(args, int);
                strTemp = itoa(iTemp, str + index);
                index += strlen(strTemp);
                i++;
                break;
            case 's': /* 字符串 */
            case 'S':
                strTemp = va_arg(args, char *);
                strcpy(str + index, strTemp);
                index += strlen(strTemp);
                i++;
                break;
            default:
                str[index++] = fmt[i];
            }
        }
    }
    str[index] = '\0';
    va_end(args);

    printf(str);
}

int main()
{
    myPrint("Version: %d; Modifier: %s\n", -958, "hallo world!");
    return 0;
}
