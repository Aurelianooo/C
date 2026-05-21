#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "common.h"

// type of optimizer
typedef enum
{
    OPT_SGD,
    OPT_SGD_MOMENTUM,
    OPT_ADAM // TODO
} OptimizerType;

typedef struct Optimizer
{
    float lr;
    float momentum;
    float weight_decay; // L2 regularization coefficient

    void (*update_weight)(struct Optimizer *self, float *w, float *dw, float *w_momentum);
    void (*update_bias)(struct Optimizer *self, float *b, float db, float *b_momentum);
    void (*set_lr)(struct Optimizer *self, float new_lr);
    void (*set_weight_decay)(struct Optimizer* self, float new_weight_decay);
} Optimizer;

// Factory Method: create instance of optimizer
Optimizer *create_optimizer(OptimizerType type, float lr, float momentum, float weight_decay);
void destroy_optimizer(Optimizer *opt);

#endif
