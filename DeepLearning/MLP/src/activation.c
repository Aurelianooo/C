#include "activation.h"

// RELU
static float relu_forward(float z) { return z > 0 ? z : 0.0f; }
static float relu_backward(float z, float a)
{
    (void)a;
    return z >= 0.0f ? 1.0f : 0.0f;
}

// Sigmoid
static float sigmoid_forward(float z) { return 1.0f / (1.0f + exp(-z)); }
static float sigmoid_backward(float z, float a)
{
    (void)z;
    return a * (1.0f - a);
}

// None
static float none_forward(float z)
{
    return z;
}

static float none_backward(float z, float a)
{
    (void)z;
    (void)a;
    return 1.0f;
}

// Factory Method
Activation *create_activation(ActivationType type)
{
    Activation *act = (Activation *)malloc(sizeof(Activation));
    if (!act)
    {
        fprintf(stderr, "Memory allocation failed for [Activation]\n");
        return NULL;
    }

    switch (type)
    {
    case ACT_RELU:
        act->forward = relu_forward;
        act->backward = relu_backward;
        break;
    case ACT_SIGMOID:
        act->forward = sigmoid_forward;
        act->backward = sigmoid_backward;
        break;
    case ACT_NONE:
        act->forward = none_forward;
        act->backward = none_backward;
        break;
    default:
        fprintf(stderr, "Unsupported [Activation] type\n");
        free(act);
        return NULL;
    }

    return act;
}

void destroy_activation(Activation *act)
{
    if (act)
        free(act);
}
