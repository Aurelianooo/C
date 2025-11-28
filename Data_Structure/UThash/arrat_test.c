#include <stdio.h>
#include "src/utarray.h"

#ifdef CUSTOM_TEST
typedef struct
{
    int a;
    char *s;
} custom_t;

void custom_copy(void *_dst, const void *_src)
{
    custom_t *dst = (custom_t *)_dst, *src = (custom_t *)_src;
    dst->a = src->a;
    dst->s = src->s ? strdup(src->s) : NULL;
}

void custom_dtor(void *_elt)
{
    custom_t *elt = (custom_t *)_elt;
    if (elt->s)
        free(elt->s);
}

UT_icd custom_icd = {sizeof(custom_t), NULL, custom_copy, custom_dtor};
#endif

int main()
{
#ifdef INT_TEST
    UT_array *nums;
    int i, *p;
    utarray_new(nums, &ut_int_icd);
    for (i = 0; i < 10; i++)
    {
        utarray_push_back(nums, &i);
    }

    for (p = (int *)utarray_front(nums);
         p != NULL;
         p = (int *)utarray_next(nums, p))
    {
        printf("%d\n", *p);
    }
    utarray_free(nums);
#endif

#ifdef STRING_TEST
    UT_array *strs;
    char *s, **p;
    utarray_new(strs, &ut_str_icd);
    s = "hello";
    utarray_push_back(strs, &s);
    s = "world";
    utarray_push_back(strs, &s);
    p = NULL;
    while ((p = (char **)utarray_next(strs, p)))
    {
        printf("%s\n", *p);
    }
    utarray_free(strs);
#endif

#ifdef CUSTOM_TEST
    UT_array *customs;
    custom_t cus, *p;
    utarray_new(customs, &custom_icd);

    cus.a = 1;
    cus.s = "hello";
    utarray_push_back(customs, &cus);
    cus.a = 2;
    cus.s = "world";
    utarray_push_back(customs, &cus);

    p = NULL;
    while ((p = (custom_t *)utarray_next(customs, p)))
    {
        printf("%d %s\n", p->a, (p->s ? p->s : "NULL"));
    }
    utarray_free(customs);
#endif

    return 0;
}
