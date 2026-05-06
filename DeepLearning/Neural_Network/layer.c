#include "layer.h"

layer create_layer(int number_of_neurons)
{
    layer lay;
    lay.num_neu = number_of_neurons;
    lay.neu = (neuron *)malloc(lay.num_neu * sizeof(neuron));
    return lay;
}

void destroy_layer(layer *this)
{
    int i;
    for (i = 0; i < this->num_neu; i++)
    {
        destroy_neuron(&(this->neu[i]));
    }
    free(this->neu);
    this->neu = NULL;

    return;
}
