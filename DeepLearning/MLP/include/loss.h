#ifndef LOSS_H
#define LOSS_H

#include "common.h"

// type of loss function
typedef enum
{
    LOSS_MSE,
    LOSS_BCE
} LossType;

typedef struct Loss
{
    float (*calculate)(float y_true, float y_pred);
    float (*derivative)(float y_true, float y_pred);
} Loss;

// Factory Method: create instance of loss function
Loss *create_loss(LossType type);
void destroy_loss(Loss *loss);

#endif