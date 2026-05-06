#include "function.h"

/* ------- LOSS ------- */

float mean_square_error(float y, float yhat)
{
    return (y - yhat) * (y - yhat) / 2;
}

float binary_cross_entropy(float y, float yhat)
{
    return -(y * logf(yhat + 1e-7f) + (1 - y) * logf(1 - yhat + 1e-7f));
}

/* ------- ACTIVATION ------- */

float relu(float x)
{
    float y = x;
    if (x < 0)
    {
        y = 0.0;
    }

    return y;
}

float sigmoid(float x)
{
    float y = 0.0;
    y = 1.0 / (1.0 + exp(-x));

    return y;
}

/* ------- NORMALIZATION ------- */

float kaiming_normal(int fan_in)
{
    float y = 0.0;
    int a = 0; // for RELU, set a = 0
    float bound = 6.0 / (1 + a * a) / fan_in;
    bound = sqrt(bound);
    y = -bound + (bound + bound) * (rand() / (RAND_MAX + 1.0));

    return y;
}
