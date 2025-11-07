#include <stdio.h>

#define SWAP(a, b)          \
    do                      \
    {                       \
        typeof(a) temp = a; \
        a = b;              \
        b = temp;           \
    } while (0)

#define MAX(x, y)                      \
    ({                                 \
        typeof(x) _max1 = (x);         \
        typeof(y) _max2 = (y);         \
        (void)(&_max1 == &_max2);      \
        _max1 > _max2 ? _max1 : _max2; \
    })

int main()
{
    int a = 1;
    int b = 2;
    SWAP(a, b);
    printf("a = %d, b = %d\n", a, b);

    int result = MAX(a++, b++); // 安全，x++和y++只执行一次
    printf("result = %d, x = %d, y = %d\n", result, a, b);

    int arr[] = {1, 2, 3, 4, 5};
    typeof(arr) *ptr = &arr;
    printf("ptr[3] = %d\n", (*ptr)[3]);

    return 0;
}
