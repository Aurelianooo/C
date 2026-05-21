#include "loss.h"

// MSE
static float mse_calculate(float y, float y_hat)
{
    float diff = y - y_hat;
    return diff * diff / 2.0f;
}

static float mse_derivative(float y, float y_hat)
{
    return y_hat - y;
}

// BCE
static float bce_calculate(float y, float y_hat)
{
    float y_hat_clamped = fmaxf(EPSILON, fminf(1.0f - EPSILON, y_hat));
    return -(y * logf(y_hat_clamped) + (1.0f - y) * logf(1.0f - y_hat_clamped));
}

static float bce_derivative(float y, float y_hat)
{
    float y_hat_clamped = fmaxf(EPSILON, fminf(1.0f - EPSILON, y_hat));
    return (y_hat_clamped - y) / (y_hat_clamped * (1.0f - y_hat_clamped));
}

// Factory Method
Loss *create_loss(LossType type)
{
    Loss *loss = (Loss *)malloc(sizeof(Loss));
    if (!loss)
    {
        fprintf(stderr, "Memory allocation failed for [Loss]\n");
        return NULL;
    }

    switch (type)
    {
    case LOSS_MSE:
        loss->calculate = mse_calculate;
        loss->derivative = mse_derivative;
        break;
    case LOSS_BCE:
        loss->calculate = bce_calculate;
        loss->derivative = bce_derivative;
        break;
    default:
        fprintf(stderr, "Unsupported [Loss] type\n");
        free(loss);
        return NULL;
    }

    return loss;
}

void destroy_loss(Loss *loss)
{
    if (loss)
        free(loss);
}