#ifndef NEURON_H
#define NEURON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

typedef struct neuron_t
{
    // Forward propagation
    float actv;             // Activation output: a = f(z), where f Activate function
    float *weights;         // Output weights connecting to each neuron in the next layer
    float bias;             // Bias term added to the weighted sum
    float z;                // Weighted sum: z = sum(w * a_prev) + bias, input to activation
    float *weight_momentum; // Accumulated weight momentum for SGD with momentum
    float bias_momentum;    // Accumulated bias momentum for SGD with momentum

    // Back propagation (d prefix = partial derivative of loss)
    float dactv; // dL/da: gradient of loss w.r.t. activation, summed from next layer
    float *dw;   // dL/dw: gradient of loss w.r.t. each output weight
    float dbias; // dL/db: gradient of loss w.r.t. bias, equals dz
    float dz;    // dL/dz: gradient of loss w.r.t. weighted sum
} neuron;

neuron create_neuron(int num_weights);
void destroy_neuron(neuron *this);

#endif
