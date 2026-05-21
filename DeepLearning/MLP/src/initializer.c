#include "initializer.h"

// Kaiming normalization
static float kaiming_normal_initialize(Initializer *self)
{
    float bound = sqrtf(6.0f / ((1.0f + self->a * self->a) * self->fan_in));
    return -bound + 2.0f * bound * ((float)rand() / (RAND_MAX + 1.0f));
}

// constant
static float constant_initialize(Initializer *self)
{
    (void)self;
    return 0.01f;
}

// Factory Method
Initializer *create_initializer(InitializerType type, int fan_in, float a)
{
    Initializer *init = (Initializer *)malloc(sizeof(Initializer));
    if (!init)
    {
        fprintf(stderr, "Memory allocation failed for [Initializer]\n");
        return NULL;
    }

    init->fan_in = fan_in;
    init->a = a;

    switch (type)
    {
    case INIT_KAIMING_NORMAL:
        init->initialize = kaiming_normal_initialize;
        break;
    case INIT_CONSTANT:
        init->initialize = constant_initialize;
        break;
    default:
        fprintf(stderr, "Unsupported [Initializer] type\n");
        free(init);
        return NULL;
    }

    return init;
}

void destroy_initializer(Initializer *init)
{
    if (init)
        free(init);
}
