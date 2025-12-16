#include <stdio.h>

typedef struct Calc
{
    int value;

    struct Calc *(*add)(struct Calc *this, int val);
    struct Calc *(*sub)(struct Calc *this, int val);
    struct Calc *(*mult)(struct Calc *this, int val);
} Calc;

Calc *add(Calc *this, int val)
{
    this->value = this->value + val;
    return this;
}

Calc *sub(Calc *this, int val)
{
    this->value = this->value - val;
    return this;
}

Calc *mult(Calc *this, int val)
{
    this->value = this->value * val;
    return this;
}

int main()
{
    Calc calc;
    calc.value = 100;
    calc.add = add;
    calc.sub = sub;
    calc.mult = mult;

    (*(*calc.add(&calc, 1)).sub(&calc, 2)).mult(&calc, 2);
    printf("%d\n", calc.value);

    return 0;
}
