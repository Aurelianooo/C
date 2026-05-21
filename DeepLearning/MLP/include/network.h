#ifndef NETWORK_H
#define NETWORK_H

#include "layer.h"
#include "loss.h"
#include "optimizer.h"
#include "initializer.h"

typedef struct
{
    int num_layers;
    int *num_neurons;
    float initial_lr;
    float momentum;
    int epoch;
    float lr_decay_epoch_ratio;
    float lr_decay_factor;
} MLPParams;

typedef struct MLP
{
    int num_layers;
    Layer **layers;
    Loss *loss;
    Optimizer *opt;
    float total_loss;

    void (*forward)(struct MLP *self, float *input);
    void (*backward)(struct MLP *self, float *y);
    void (*update_params)(struct MLP *self);
    float (*train_step)(struct MLP *self, float *input, float *y);
    float (*predict)(struct MLP *self, float *input);
    void (*reset_loss)(struct MLP *self);
    void (*destroy)(struct MLP *self);
} MLP;

// CTOR
MLP *mlp_create(MLPParams *params, Loss *loss, Optimizer *opt);

#endif
