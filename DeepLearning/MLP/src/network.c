#include "network.h"

static void mlp_forward(MLP *self, float *input)
{

    Layer *input_layer = self->layers[0];
    for (int j = 0; j < input_layer->num_neu; j++)
    {
        input_layer->neurons[j]->actv = input[j];
    }

    for (int i = 1; i < self->num_layers; i++)
    {
        Layer *curr_layer = self->layers[i];
        Layer *prev_layer = self->layers[i - 1];

        for (int j = 0; j < curr_layer->num_neu; j++)
        {
            Neuron *curr_neu = curr_layer->neurons[j];
            curr_neu->z = curr_neu->bias;
            for (int k = 0; k < prev_layer->num_neu; k++)
            {
                curr_neu->z += prev_layer->neurons[k]->weights[j] * prev_layer->neurons[k]->actv;
            }
            curr_neu->actv = curr_layer->activation->forward(curr_neu->z);
        }
    }
}

static void mlp_backward(MLP *self, float *y)
{

    for (int i = 0; i < self->num_layers; i++)
    {
        self->layers[i]->reset_gradients(self->layers[i]);
    }

    // Output layer
    Layer *output_layer = self->layers[self->num_layers - 1];
    Layer *prev_output_layer = self->layers[self->num_layers - 2];

    for (int j = 0; j < output_layer->num_neu; j++)
    {
        Neuron *output_neu = output_layer->neurons[j];

        float y_hat = output_neu->actv;
        output_neu->dz = self->loss->derivative(y[j], y_hat) *
                         output_layer->activation->backward(output_neu->z, y_hat);
        output_neu->dbias = output_neu->dz;
        for (int k = 0; k < prev_output_layer->num_neu; k++)
        {
            Neuron *prev_neu = prev_output_layer->neurons[k];
            prev_neu->dw[j] = output_neu->dz * prev_neu->actv;
            prev_neu->dactv += prev_neu->weights[j] * output_neu->dz;
        }
    }

    // Hidden layer
    for (int i = self->num_layers - 2; i > 0; i--)
    {
        Layer *curr_layer = self->layers[i];
        Layer *prev_layer = self->layers[i - 1];

        for (int j = 0; j < curr_layer->num_neu; j++)
        {
            Neuron *curr_neu = curr_layer->neurons[j];
            curr_neu->dz = curr_neu->dactv *
                           curr_layer->activation->backward(curr_neu->z, curr_neu->actv);
            curr_neu->dbias = curr_neu->dz;
            for (int k = 0; k < prev_layer->num_neu; k++)
            {
                Neuron *prev_neu = prev_layer->neurons[k];
                prev_neu->dw[j] = curr_neu->dz * prev_neu->actv;
                if (i > 1)
                    prev_neu->dactv += prev_neu->weights[j] * curr_neu->dz;
            }
        }
    }
}

static void mlp_update_params(MLP *self)
{
    for (int i = 0; i < self->num_layers; i++)
    {
        self->layers[i]->update_params(self->layers[i], self->opt);
    }
}

static float mlp_train_step(MLP *self, float *input, float *y)
{
    self->forward(self, input);

    float y_hat = self->layers[self->num_layers - 1]->neurons[0]->actv;
    float loss = self->loss->calculate(y[0], y_hat);
    self->total_loss += loss;

    self->backward(self, y);
    self->update_params(self);

    return loss;
}

static float mlp_predict(MLP *self, float *input)
{
    self->forward(self, input);
    return self->layers[self->num_layers - 1]->neurons[0]->actv;
}

static void mlp_reset_loss(MLP *self)
{
    self->total_loss = 0.0f;
}

static void mlp_destroy(MLP *self)
{
    if (!self)
        return;

    if (self->layers)
    {
        for (int i = 0; i < self->num_layers; i++)
        {
            layer_destroy(self->layers[i]);
        }
        free(self->layers);
    }

    free(self);
}

MLP *mlp_create(MLPParams *params, Loss *loss, Optimizer *opt)
{
    if (!params || !loss || !opt || params->num_layers < 2)
    {
        fprintf(stderr, "Invalid parameters for MLP creation\n");
        return NULL;
    }

    MLP *self = (MLP *)malloc(sizeof(MLP));
    if (!self)
    {
        fprintf(stderr, "Memory allocation failed for [MLP]\n");
        return NULL;
    }

    self->num_layers = params->num_layers;
    self->loss = loss;
    self->opt = opt;
    self->total_loss = 0.0f;
    self->layers = (Layer **)malloc(params->num_layers * sizeof(Layer *));

    if (!self->layers)
    {
        fprintf(stderr, "Memory allocation failed for [MLP layers]\n");
        mlp_destroy(self);
        return NULL;
    }

    // Input layer
    self->layers[0] = layer_create(
        params->num_neurons[0],
        params->num_neurons[1],
        ACT_NONE);

    // Hidden layer
    for (int i = 1; i < params->num_layers - 1; i++)
    {
        self->layers[i] = layer_create(
            params->num_neurons[i],
            params->num_neurons[i + 1],
            ACT_RELU);
    }

    // Output layer
    self->layers[params->num_layers - 1] = layer_create(
        params->num_neurons[params->num_layers - 1],
        0,
        ACT_SIGMOID);

    for (int i = 0; i < params->num_layers; i++)
    {
        if (!self->layers[i])
        {
            fprintf(stderr, "Failed to create layer %d\n", i);
            mlp_destroy(self);
            return NULL;
        }
    }

    // Initialize weights（Kaiming norm）
    for (int i = 0; i < params->num_layers - 1; i++)
    {
        Initializer *init = create_initializer(INIT_KAIMING_NORMAL, params->num_neurons[i], 0.0f);
        if (!init)
        {
            fprintf(stderr, "Failed to create initializer for layer %d\n", i);
            mlp_destroy(self);
            return NULL;
        }

        for (int j = 0; j < self->layers[i]->num_neu; j++)
        {
            self->layers[i]->neurons[j]->init_weights(self->layers[i]->neurons[j], init);
        }

        destroy_initializer(init);
    }

    self->forward = mlp_forward;
    self->backward = mlp_backward;
    self->update_params = mlp_update_params;
    self->train_step = mlp_train_step;
    self->predict = mlp_predict;
    self->reset_loss = mlp_reset_loss;
    self->destroy = mlp_destroy;

    return self;
}
