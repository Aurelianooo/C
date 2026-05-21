#ifndef ACTIVATION_H
#define ACTIVATION_H

#include "common.h"

// type of activation function
typedef enum
{
    ACT_RELU,
    ACT_SIGMOID,
    ACT_TANH, // TODO
    ACT_NONE
} ActivationType;

typedef struct
{
    float (*forward)(float z);
    float (*backward)(float z, float a);
} Activation;

// Factory Method: create instance of activation function
Activation *create_activation(ActivationType type);
void destroy_activation(Activation *act);

#endif