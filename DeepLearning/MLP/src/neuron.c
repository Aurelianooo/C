#include "neuron.h"

static void neuron_init_weights(Neuron *self, Initializer *init)
{
    for (int k = 0; k < self->num_weights; k++)
    {
        self->weights[k] = init->initialize(init);
    }
    self->bias = 0.0f;
}

static void neuron_update_params(Neuron *self, Optimizer *opt)
{
    for (int k = 0; k < self->num_weights; k++)
    {
        opt->update_weight(opt, &self->weights[k], &self->dw[k], &self->weight_momentum[k]);
    }
    opt->update_bias(opt, &self->bias, self->dbias, &self->bias_momentum);
}

static void neuron_reset_gradients(Neuron *self)
{
    self->dactv = 0.0f;
    self->dbias = 0.0f;
    self->dz = 0.0f;
    memset(self->dw, 0, self->num_weights * sizeof(float));
}

Neuron *neuron_create(int num_weights)
{
    if (num_weights < 0)
    {
        fprintf(stderr, "Invalid number of weights: %d\n", num_weights);
        return NULL;
    }

    Neuron *self = (Neuron *)malloc(sizeof(Neuron));
    if (!self)
    {
        fprintf(stderr, "Memory allocation failed for [Neuron]\n");
        return NULL;
    }

    self->num_weights = num_weights;
    self->actv = 0.0f;
    self->bias = 0.0f;
    self->z = 0.0f;
    self->bias_momentum = 0.0f;
    self->dactv = 0.0f;
    self->dbias = 0.0f;
    self->dz = 0.0f;

    if (num_weights > 0)
    {
        self->weights = (float *)malloc(num_weights * sizeof(float));
        self->weight_momentum = (float *)calloc(num_weights, sizeof(float));
        self->dw = (float *)calloc(num_weights, sizeof(float));

        if (!self->weights || !self->weight_momentum || !self->dw)
        {
            fprintf(stderr, "Memory allocation failed for Neuron arrays\n");
            neuron_destroy(self);
            return NULL;
        }
    }
    else
    {
        self->weights = NULL;
        self->weight_momentum = NULL;
        self->dw = NULL;
    }

    self->init_weights = neuron_init_weights;
    self->update_params = neuron_update_params;
    self->reset_gradients = neuron_reset_gradients;

    return self;
}

void neuron_destroy(Neuron *self)
{
    if (!self)
        return;

    if (self->weights)
        free(self->weights);
    if (self->weight_momentum)
        free(self->weight_momentum);
    if (self->dw)
        free(self->dw);

    free(self);
}
