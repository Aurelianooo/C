#include <stdio.h>
#include <math.h>

// #define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX(a,b)    \
    ({\
            typeof(a) _a = a;\
            typeof(b) _b = b;\
            (void)(&_a == &_b);\
            ((_a)>(_b) ? (_a):(_b));\
    })
                        

int main()
{
	printf("fmax (100.0, 1.0) = %f\n", fmax(100.0, 1.0));
	printf("fmax (-100.0, 1.0) = %f\n", fmax(-100.0, 1.0));
	printf("fmax (-100.0, -1.0) = %f\n", fmax(-100.0, -1.0));

	printf("MIN (3,8) = %d\n", MIN(3, 8));
	printf("MAX (3,8) = %f\n", MAX(3, 8.0));
	return 0;
}