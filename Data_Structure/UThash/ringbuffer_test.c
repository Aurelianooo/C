#include <stdio.h>
#include "src/utringbuffer.h"

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
    free(elt->s);
}

UT_icd custom_icd = {sizeof(custom_t), NULL, custom_copy, custom_dtor};
#endif

int main()
{
#ifdef BASIC_TEST
    UT_ringbuffer *nums;
    int i, *p;

    utringbuffer_new(nums, 7, &ut_int_icd);
    for (i = 0; i < 10; i++)
    {
        utringbuffer_push_back(nums, &i);
    }

    for (p = (int *)utringbuffer_front(nums);
         p != NULL;
         p = (int *)utringbuffer_next(nums, p))
    {
        printf("%d ", *p);
    }
    printf("\n--------------\n");

    for (i = 0; i < utringbuffer_len(nums); i++)
    {
        p = utringbuffer_eltptr(nums, i);
        printf("%d ", *p);
    }
    printf("\n");

    utringbuffer_free(nums);
#endif

#ifdef STRING_TEST
    UT_ringbuffer *strs;
    char *s, **p;

    utringbuffer_new(strs, 4, &ut_str_icd);

    s = "hello";
    utringbuffer_push_back(strs, &s);
    s = "world";
    utringbuffer_push_back(strs, &s);
    p = NULL;
    while ((p = (char **)utringbuffer_next(strs, p)))
    {
        printf("%s\n", *p);
    }

    utringbuffer_free(strs);
#endif

#ifdef CUSTOM_TEST
    UT_ringbuffer *customs;
    custom_t cus, *p;
    utringbuffer_new(customs, 4, &custom_icd);

    cus.a = 1;
    cus.s = "hello";
    utringbuffer_push_back(customs, &cus);
    cus.a = 2;
    cus.s = "world";
    utringbuffer_push_back(customs, &cus);

    p = NULL;
    while ((p = (custom_t *)utringbuffer_next(customs, p)))
    {
        printf("%d %s\n", p->a, (p->s ? p->s : "NULL"));
    }

    utringbuffer_free(customs);
#endif
    return 0;
}
