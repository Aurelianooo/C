#include "neuron.h"


neuron create_neuron(int num_weights)
{
	neuron neu;

	neu.actv = 0.0;
	neu.weights = (float*) malloc(num_weights * sizeof(float));
	neu.bias= 0.0;
	neu.z = 0.0;
	neu.weight_momentum = (float*) calloc(num_weights, sizeof(float));
	neu.bias_momentum = 0.0;

	neu.dactv = 0.0;
	neu.dw = (float*) malloc(num_weights * sizeof(float));
	neu.dbias = 0.0;
	neu.dz = 0.0;

	return neu;
}

void destroy_neuron(neuron *this)
{
	free(this->weights);
	this->weights = NULL;
	free(this->weight_momentum);
	this->weight_momentum = NULL;
	free(this->dw);
	this->dw = NULL;

	return;
}
