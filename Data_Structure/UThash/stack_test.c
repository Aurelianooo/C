#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include "src/utstack.h"

typedef struct el
{
    int id, score;
    struct el *next;
    struct el *next2;
} el;

int main()
{
    el a = {1, 100, NULL, NULL};
    el b = {2, 200, NULL, NULL};
    el c = {3, 300, NULL, NULL};
    el d = {4, 400, NULL, NULL};

    el *stack1 = NULL;
    el *stack2 = NULL;
    el *dummy;
    int size1;
    size_t size2;

    STACK_COUNT(stack1, dummy, size1);
    assert(size1 == 0);
    STACK_COUNT(stack2, dummy, size2);
    assert(size2 == 0);

    STACK_PUSH(stack1, &a);
    STACK_COUNT(stack1, dummy, size1);
    assert(size1 == 1);
    STACK_PUSH(stack1, &b);
    STACK_COUNT(stack1, dummy, size1);
    assert(size1 == 2);
    STACK_PUSH(stack1, &c);
    STACK_PUSH2(stack1, &d, next);
    STACK_COUNT(stack1, dummy, size1);
    assert(!STACK_EMPTY(stack1));
    printf("size1 = %d\n", size1);
    assert(stack1 == &d);

    STACK_PUSH2(stack2, &a, next2);
    assert(stack2 == &a);
    assert(a.next2 == NULL);
    STACK_PUSH2(stack2, &d, next2);
    assert(stack2 == &d);
    assert(d.next2 == &a);
    STACK_COUNT2(stack2, dummy, size2, next2);
    printf("size2 = %lu\n", size2);
    assert(!STACK_EMPTY(stack2));
    assert(stack2 == &d);

    STACK_POP(stack1, dummy);
    assert(stack1 == &c);
    assert(dummy == &d);
    STACK_POP(stack1, dummy);
    assert(stack1 == &b);
    assert(dummy == &c);
    STACK_POP(stack1, dummy);
    assert(stack1 == &a);
    assert(dummy == &b);
    STACK_COUNT(stack1, dummy, size1);
    printf("size1 = %d\n", size1);
    STACK_POP(stack1, dummy);
    assert(stack1 == NULL);
    assert(dummy == &a);

    assert(STACK_TOP(stack2) == &d);
    while (!STACK_EMPTY(stack2))
    {
        STACK_POP2(stack2, dummy, next2);
    }

    assert(STACK_EMPTY(stack1));
    assert(STACK_EMPTY(stack2));

    return 0;
}
