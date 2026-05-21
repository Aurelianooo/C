#ifndef INITIALIZER_H
#define INITIALIZER_H

#include "common.h"

// type of initializer
typedef enum
{
    INIT_KAIMING_NORMAL,
    INIT_CONSTANT
} InitializerType;

typedef struct Initializer
{
    int fan_in;
    float a;

    float (*initialize)(struct Initializer *self);
} Initializer;

// Factory Method: create instance of initializer
Initializer *create_initializer(InitializerType type, int fan_in, float a);
void destroy_initializer(Initializer *init);

#endif
