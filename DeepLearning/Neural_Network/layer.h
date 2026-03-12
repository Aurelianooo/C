#ifndef LAYER_H
#define LAYER_H

#include "neuron.h"


typedef struct layer_t
{
	int num_neu;
	neuron *neu; 
} layer;

layer create_layer(int num_neurons);
void destroy_layer(layer *this);

#endif