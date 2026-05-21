#ifndef LAYER_H
#define LAYER_H

#include "neuron.h"

typedef struct Layer
{
    int num_neu;
    Neuron **neurons;
    Activation *activation;

    void (*forward)(struct Layer *self, float *prev_actvs);
    void (*backward)(struct Layer *self);
    void (*update_params)(struct Layer *self, Optimizer *opt);
    void (*reset_gradients)(struct Layer *self);
} Layer;

// CTOR
Layer *layer_create(int num_neu, int num_weights, ActivationType act_type);
// DTOR
void layer_destroy(Layer *self);

#endif
