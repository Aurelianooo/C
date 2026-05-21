#include "layer.h"

static void layer_update_params(Layer *self, Optimizer *opt)
{
    for (int j = 0; j < self->num_neu; j++)
    {
        self->neurons[j]->update_params(self->neurons[j], opt);
    }
}

static void layer_reset_gradients(Layer *self)
{
    for (int j = 0; j < self->num_neu; j++)
    {
        self->neurons[j]->reset_gradients(self->neurons[j]);
    }
}

Layer *layer_create(int num_neu, int num_weights, ActivationType act_type)
{
    if (num_neu <= 0)
    {
        fprintf(stderr, "Invalid number of neurons: %d\n", num_neu);
        return NULL;
    }

    Layer *self = (Layer *)malloc(sizeof(Layer));
    if (!self)
    {
        fprintf(stderr, "Memory allocation failed for [Layer]\n");
        return NULL;
    }

    self->num_neu = num_neu;
    self->neurons = (Neuron **)malloc(num_neu * sizeof(Neuron *));
    self->activation = create_activation(act_type);

    if (!self->neurons || !self->activation)
    {
        fprintf(stderr, "Memory allocation failed for [Layer components]\n");
        layer_destroy(self);
        return NULL;
    }

    for (int j = 0; j < num_neu; j++)
    {
        self->neurons[j] = neuron_create(num_weights);
        if (!self->neurons[j])
        {
            fprintf(stderr, "Failed to create neuron %d in layer\n", j);
            layer_destroy(self);
            return NULL;
        }
    }

    self->update_params = layer_update_params;
    self->reset_gradients = layer_reset_gradients;

    return self;
}

void layer_destroy(Layer *self)
{
    if (!self)
        return;

    if (self->neurons)
    {
        for (int j = 0; j < self->num_neu; j++)
        {
            neuron_destroy(self->neurons[j]);
        }
        free(self->neurons);
    }

    destroy_activation(self->activation);
    free(self);
}
