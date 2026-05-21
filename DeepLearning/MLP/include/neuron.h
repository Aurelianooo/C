#ifndef NEURON_H
#define NEURON_H

#include "common.h"
#include "activation.h"
#include "optimizer.h"
#include "initializer.h"

typedef struct Neuron
{

    int num_weights;
    float actv;
    float *weights;
    float bias;
    float z;
    float *weight_momentum;
    float bias_momentum;

    // derivation
    float dactv;
    float *dw;
    float dbias;
    float dz;

    void (*init_weights)(struct Neuron *self, Initializer *init);
    void (*compute_z)(struct Neuron *self, float *prev_actvs, int prev_num);
    void (*compute_actv)(struct Neuron *self, Activation *act);
    void (*update_params)(struct Neuron *self, Optimizer *opt);
    void (*reset_gradients)(struct Neuron *self);
} Neuron;

// CTOR
Neuron *neuron_create(int num_weights);
// DTOR
void neuron_destroy(Neuron *self);

#endif
