#ifndef NETWORK_H
#define NETWORK_H

#include "layer.h"


/* Define success and errors */
#define SUCCESS 0
#define ERR_INIT 1
#define ERR_CREATE_NETWORK 2
#define ERR_INIT_WEIGHTS 3
#define ERR_LOAD_PARAMS 4
#define ERR_INIT_DATASET 5

typedef struct hyper_parameter_t
{
    int num_layers;
    int *num_neurons;

    float momentum;
    float learning_rate;
    // int batch_size;
    int epoch;
} hyper_parameter;

typedef struct neural_network_t
{
    int num_lay;
    layer *lay;
    hyper_parameter *hp;

    float *cost;
    float full_cost;
} neural_network;

typedef struct data_set_t
{
    int num_training_ex;
    int num_test_ex;
    int num_item;
    
    const char *dataset_path;
    float **train_set;
    float **test_set;
} data_set;


int init_network(neural_network* nn, hyper_parameter *hp);
int create_network(neural_network* nn);
float kaiming_normal(int fan_in);
int init_weights(neural_network *nn);
void shuffle_dataset(float **p, int col, int row);
int init_dataset(data_set *ds);
void train(neural_network *nn, data_set *ds);
void feed_input(int i, neural_network *nn, float **set);
void forward_prop(neural_network *nn);
void compute_cost(int i, neural_network *nn, data_set *ds);
void back_prop(int p, neural_network *nn, data_set *ds);
void update_weights(neural_network *nn);
void test(neural_network *nn, data_set *ds);
void print_weights(neural_network *nn);
void print_actvs(neural_network *nn);
void destroy_dataset(data_set *this);
void destroy_network(neural_network *this);
void destroy_hyperparameter(hyper_parameter *this);

#endif
