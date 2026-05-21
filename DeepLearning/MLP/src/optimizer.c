#include "optimizer.h"

// SGD
static void sgd_update_weight(Optimizer *self, float *w, float *dw, float *w_momentum)
{
    (void)w_momentum;
    *w -= self->lr * (*dw);
    *w -= self->lr * self->weight_decay * (*w);
}

static void sgd_update_bias(Optimizer *self, float *b, float db, float *b_momentum)
{
    (void)b_momentum;
    *b -= self->lr * db;
}

// SGD w/ momentum
static void sgd_momentum_update_weight(Optimizer *self, float *w, float *dw, float *w_momentum)
{
    *w_momentum = self->momentum * (*w_momentum) + self->lr * (*dw);
    *w -= *w_momentum;
    *w -= self->lr * self->weight_decay * (*w);
}

static void sgd_momentum_update_bias(Optimizer *self, float *b, float db, float *b_momentum)
{
    *b_momentum = self->momentum * (*b_momentum) + self->lr * db;
    *b -= *b_momentum;
}

// learning rate
static void optimizer_set_lr(Optimizer *self, float new_lr)
{
    self->lr = new_lr;
}

// L2 regularization
static void optimizer_set_weight_decay(Optimizer* self, float new_weight_decay) {
    self->weight_decay = new_weight_decay;
}

// Factory Method
Optimizer *create_optimizer(OptimizerType type, float lr, float momentum, float weight_decay)
{
    Optimizer *opt = (Optimizer *)malloc(sizeof(Optimizer));
    if (!opt)
    {
        fprintf(stderr, "Memory allocation failed for [Optimizer]\n");
        return NULL;
    }

    opt->lr = lr;
    opt->momentum = momentum;
    opt->weight_decay = weight_decay;
    opt->set_lr = optimizer_set_lr;
    opt->set_weight_decay = optimizer_set_weight_decay;

    switch (type)
    {
    case OPT_SGD:
        opt->update_weight = sgd_update_weight;
        opt->update_bias = sgd_update_bias;
        break;
    case OPT_SGD_MOMENTUM:
        opt->update_weight = sgd_momentum_update_weight;
        opt->update_bias = sgd_momentum_update_bias;
        break;
    default:
        fprintf(stderr, "Unsupported [Optimizer] type\n");
        free(opt);
        return NULL;
    }

    return opt;
}

void destroy_optimizer(Optimizer *opt)
{
    if (opt)
        free(opt);
}
