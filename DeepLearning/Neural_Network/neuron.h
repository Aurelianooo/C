#ifndef NEURON_H
#define NEURON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>


typedef struct neuron_t
{
	float actv;
	float *weights;
	float bias;
	float z;
	float *weight_momentum;
	float bias_momentum;

	float dactv;
	float *dw;
	float dbias;
	float dz;

	// TODO: Add function pointer for destructor

} neuron;

neuron create_neuron(int num_weights);
void destroy_neuron(neuron *this);

#endif