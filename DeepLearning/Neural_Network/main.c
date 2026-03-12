#include "network.h"


int main(void)
{
    // Just srand once at beginning is OK!
    srand(time(0));

    hyper_parameter hp;
    neural_network nn;
    data_set iono;

    // Set the hyperparameters of the neural network
    hp.epoch = 300;
    hp.learning_rate = 0.1;
    hp.momentum = 0.9;
    hp.num_layers = 4;  // layer number of the neural network
    hp.num_neurons = (int*) malloc(hp.num_layers * sizeof(int));
    hp.num_neurons[0] = 34;    // input size
    hp.num_neurons[1] = 16;
    hp.num_neurons[2] = 8;
    hp.num_neurons[3] = 1;     // output size

    // Set up the dataset 
    iono.dataset_path = "../Datasets/ionosphere.data";
    iono.num_training_ex = 320;
    iono.num_test_ex = 31;
    iono.num_item = 34 + 1; // same as neural network (input size + output size)

    // Initialize neural network
    if(init_network(&nn, &hp)!= SUCCESS) {
        printf("Error in Network Initialization...\n");
        exit(0);
    }
    // print_weights(&nn);

    // Initialize dataset
    if(init_dataset(&iono)!= SUCCESS) {
        printf("Error in Dataset Initialization...\n");
        exit(0);
    }

    train(&nn, &iono);
    test(&nn, &iono);

    destroy_dataset(&iono);
    destroy_network(&nn);
    destroy_hyperparameter(&hp);

    return 0;
}

int init_network(neural_network* nn, hyper_parameter *hp)
{
    nn->num_lay = hp->num_layers;
    nn->lay = (layer *) malloc(nn->num_lay * sizeof(layer));

    nn->full_cost = 0.0;
    nn->cost = (float *) malloc(hp->num_neurons[nn->num_lay-1] * sizeof(float));
    memset(nn->cost, 0, hp->num_neurons[nn->num_lay-1]*sizeof(float));
    nn->hp = hp;

    if(create_network(nn) != SUCCESS)
    {
        printf("Error in creating architecture...\n");
        return ERR_INIT;
    }

    printf("Neural Network Created Successfully...\n");
    return SUCCESS;
}

// Create Neural Network Architecture
int create_network(neural_network* nn)
{
    int i, j;
    for(i=0;i<nn->num_lay;i++)
    {
        nn->lay[i] = create_layer(nn->hp->num_neurons[i]);
        for(j=0;j<nn->lay[i].num_neu;j++)
        {
            if(i < (nn->num_lay-1)) 
            {   // The number of weights for the neurons in this layer of the neural network
                // should be equal to the number of neurons in the next layer.
                nn->lay[i].neu[j] = create_neuron(nn->hp->num_neurons[i+1]);
            }

        }
    }

    // Initialize the weights
    if(init_weights(nn) != SUCCESS)
    {
        printf("Error Initilizing weights...\n");
        return ERR_CREATE_NETWORK;
    }

    return SUCCESS;
}

float kaiming_normal(int fan_in)
{
    float res = 0.0;
    int a = 0;  // for RELU, set a = 0
    float bound = 6.0 / (1 + a*a) / fan_in;
    bound = sqrt(bound);
    res = -bound + (bound + bound) * (rand() / (RAND_MAX + 1.0));

    return res;
}

int init_weights(neural_network *nn)
{
    int i,j,k;

    if(nn->lay == NULL)
    {
        printf("No layers in Neural Network...\n");
        return ERR_INIT_WEIGHTS;
    }

    printf("Initializing weights...\n");

    for(i=0;i<nn->num_lay-1;i++)
    {
        for(j=0;j<nn->lay[i].num_neu;j++)
        {
            for(k=0;k<nn->lay[i+1].num_neu;k++)
            {
                // Initialize Output Weights for each neuron
                nn->lay[i].neu[j].weights[k] = kaiming_normal(nn->lay[i].num_neu);
                // lay[i].neu[j].weights[k] = 0.01;
                nn->lay[i].neu[j].dw[k] = 0.0;
            }

            if(i>0) 
            {
                nn->lay[i].neu[j].bias = 0.0;
            }
        }
    }
    
    for (j=0; j<nn->lay[nn->num_lay-1].num_neu; j++)
    {
        nn->lay[nn->num_lay-1].neu[j].bias = 0.0;
    }

    return SUCCESS;
}

// Shuffle Dataset before split into training and test sets
void shuffle_dataset(float **p, int col, int row)
{
    int i, j, idx;
    float *temp = (float*)malloc(col * sizeof(float));
    
    for(i=0;i<row-1;i++){
        idx = i+rand()%(row-1-i);
        for(j=0;j<col;j++)
            temp[j] = p[i][j];

        for(j=0;j<col;j++)
            p[i][j] = p[idx][j];

        for(j=0;j<col;j++)
            p[idx][j] = temp[j];
    }

    free(temp);
    return;
}

int init_dataset(data_set *ds)
{
    int i, j;
    int cols = ds->num_item;
    int rows = ds->num_training_ex + ds->num_test_ex;

    // All data from dataset
    float **all_data = (float**) malloc(rows * sizeof(float*));
    for(i=0;i<rows;i++)
    {
        all_data[i] = (float*)malloc(cols * sizeof(float));
    }

    // Training data array
    ds->train_set = (float**) malloc(ds->num_training_ex * sizeof(float*));
    for(i=0;i<ds->num_training_ex;i++)
    {
        ds->train_set[i] = (float*)malloc(cols * sizeof(float));
    }

    // Test data array
    ds->test_set = (float**) malloc(ds->num_test_ex * sizeof(float*));
    for(i=0;i<ds->num_test_ex;i++)
    {
        ds->test_set[i] = (float*)malloc(cols * sizeof(float));
    }

    printf("Initializing dataset...\n");
    char buffer[512];
    FILE *fptr;
    if ((fptr = fopen(ds->dataset_path, "r")) == NULL) {
        printf("Error in initializing dataset...\n");
        return ERR_INIT_DATASET;
    }

    char *temp = NULL;
    i = 0;
    while(fgets(buffer, 512, fptr)!=NULL && i < (ds->num_training_ex + ds->num_test_ex)){
        temp = strtok(buffer,",");
        j = 0;
        while(temp && j< cols){
            all_data[i][j] = atof(temp);
            temp = strtok(NULL,",");
            j++;
        }
        i++;
    }

    shuffle_dataset(all_data, cols, rows);

    for(i=0;i<ds->num_training_ex;i++){
        for(j=0;j<cols;j++){
            ds->train_set[i][j] = all_data[i][j];
        }
    }
    for(;i<rows;i++){
        for(j=0;j<cols;j++){
            ds->test_set[i-ds->num_training_ex][j] = all_data[i][j];
        }
    }

    for(i=0;i<rows;i++){
        free(all_data[i]);
        all_data[i] = NULL;
    }
    free(all_data);
    all_data = NULL;
    fclose(fptr);
    printf("Dataset initialization Successfully...\n");
    return SUCCESS;
}

// Train Neural Network
void train(neural_network *nn, data_set *ds)
{
    int i, j;
    struct timeval start, end;
    long long total_time;

    // Stochastic Gradient Descent with momentum
    for(j=0;j<nn->hp->epoch;j++){
        if(j == (int)round(nn->hp->epoch * 0.618))
            nn->hp->learning_rate = nn->hp->learning_rate * 0.1;
        
        nn->full_cost = 0.0;
        gettimeofday(&start, NULL);
        for(i=0;i<ds->num_training_ex;i++)
        {
            feed_input(i, nn, ds->train_set);
            forward_prop(nn);
            compute_cost(i, nn, ds);
            back_prop(i, nn, ds);
            update_weights(nn);
        }
        gettimeofday(&end, NULL);
        total_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        total_time /= 1000; // get the epoch run time by millisecond
        printf("epoch [%d] get cost [%f], total time is [%lld] ms\n", j, nn->full_cost, total_time);
        // print_weights(&nn);
        // shuffle dataset to realize stochastic
        shuffle_dataset(ds->train_set, nn->lay[0].num_neu + nn->lay[nn->num_lay-1].num_neu, ds->num_training_ex);
    }
}

// Feed inputs to input layer
void feed_input(int idx, neural_network *nn, float **set)
{
    int j;
    for(j=0;j<nn->lay[0].num_neu;j++)
        nn->lay[0].neu[j].actv = set[idx][j];
}

void forward_prop(neural_network *nn)
{
    int i,j,k;

    for(i=1;i<nn->num_lay;i++)
    {   
        for(j=0;j<nn->lay[i].num_neu;j++)
        {
            nn->lay[i].neu[j].z = nn->lay[i].neu[j].bias;
            for(k=0;k<nn->lay[i-1].num_neu;k++)
            {
                nn->lay[i].neu[j].z  = nn->lay[i].neu[j].z + ((nn->lay[i-1].neu[k].weights[j]) * (nn->lay[i-1].neu[k].actv));
            }
            
            if(i < nn->num_lay-1){   // Relu Activation Function for Hidden Layers
                if((nn->lay[i].neu[j].z) < 0){
                    nn->lay[i].neu[j].actv = 0.0;
                }else{
                    nn->lay[i].neu[j].actv = nn->lay[i].neu[j].z;
                }
            }else{   // Sigmoid Activation function for Output Layer
                nn->lay[i].neu[j].actv = 1.0/(1.0 + exp(-nn->lay[i].neu[j].z));
            }
        }
    }
}

// Compute Total Cost
void compute_cost(int i, neural_network *nn, data_set *ds)
{
    int j;
    float tmpcost=0;
    float tcost=0;

    // Calculate Mean Squared Error Loss
    for(j=0;j<nn->lay[nn->num_lay-1].num_neu;j++){
        tmpcost = ds->train_set[i][j+nn->lay[0].num_neu] - nn->lay[nn->num_lay-1].neu[j].actv;
        nn->cost[j] = (tmpcost * tmpcost)/2;
        tcost = tcost + nn->cost[j];
    }
    nn->full_cost += tcost;
    if(i == ds->num_training_ex -1)
        nn->full_cost /= ds->num_training_ex;

    return;
}

// Back Propogation Error
void back_prop(int p, neural_network *nn, data_set *ds)
{
    int i,j,k;

    // Output Layer
    for(j=0;j<nn->lay[nn->num_lay-1].num_neu;j++)
    {           
        nn->lay[nn->num_lay-1].neu[j].dz = (nn->lay[nn->num_lay-1].neu[j].actv - ds->train_set[p][j+nn->lay[0].num_neu]) * (nn->lay[nn->num_lay-1].neu[j].actv) * (1 - nn->lay[nn->num_lay-1].neu[j].actv);

        for(k=0;k<nn->lay[nn->num_lay-2].num_neu;k++)
        {   
            nn->lay[nn->num_lay-2].neu[k].dw[j] = nn->lay[nn->num_lay-1].neu[j].dz * nn->lay[nn->num_lay-2].neu[k].actv;
            nn->lay[nn->num_lay-2].neu[k].weight_momentum[j] = nn->hp->momentum * nn->lay[nn->num_lay-2].neu[k].weight_momentum[j] + nn->hp->learning_rate * nn->lay[nn->num_lay-2].neu[k].dw[j];
            nn->lay[nn->num_lay-2].neu[k].dactv = nn->lay[nn->num_lay-2].neu[k].weights[j] * nn->lay[nn->num_lay-1].neu[j].dz;
        }

        nn->lay[nn->num_lay-1].neu[j].dbias = nn->lay[nn->num_lay-1].neu[j].dz;
        nn->lay[nn->num_lay-1].neu[j].bias_momentum = nn->hp->momentum * nn->lay[nn->num_lay-1].neu[j].bias_momentum + nn->hp->learning_rate * nn->lay[nn->num_lay-1].neu[j].dbias;
    }

    // Hidden Layers
    for(i=nn->num_lay-2;i>0;i--)
    {
        for(j=0;j<nn->lay[i].num_neu;j++)
        {
            if(nn->lay[i].neu[j].z >= 0.0)
            {
                nn->lay[i].neu[j].dz = nn->lay[i].neu[j].dactv;
            }
            else
            {
                nn->lay[i].neu[j].dz = 0.0;
            }

            for(k=0;k<nn->lay[i-1].num_neu;k++)
            {
                nn->lay[i-1].neu[k].dw[j] = nn->lay[i].neu[j].dz * nn->lay[i-1].neu[k].actv;    
                nn->lay[i-1].neu[k].weight_momentum[j] = nn->hp->momentum * nn->lay[i-1].neu[k].weight_momentum[j] + nn->hp->learning_rate * nn->lay[i-1].neu[k].dw[j];
                if(i>1)
                {
                    nn->lay[i-1].neu[k].dactv = nn->lay[i-1].neu[k].weights[j] * nn->lay[i].neu[j].dz;
                }
            }

            nn->lay[i].neu[j].dbias = nn->lay[i].neu[j].dz;
            nn->lay[i].neu[j].bias_momentum = nn->hp->momentum * nn->lay[i].neu[j].bias_momentum + nn->hp->learning_rate * nn->lay[i].neu[j].dbias;
        }
    }
}

void update_weights(neural_network *nn)
{
    int i,j,k;

    for(i=0;i<nn->num_lay-1;i++)
    {
        for(j=0;j<nn->lay[i].num_neu;j++)
        {
            for(k=0;k<nn->lay[i+1].num_neu;k++)
            {
                // Update Weights
                nn->lay[i].neu[j].weights[k] = nn->lay[i].neu[j].weights[k] - nn->lay[i].neu[j].weight_momentum[k];
            }
            
            // Update Bias
            nn->lay[i].neu[j].bias = nn->lay[i].neu[j].bias - nn->lay[i].neu[j].bias_momentum;
        }
    }   
}

// Test the trained network
// Applicable only to single output
void test(neural_network *nn, data_set *ds) 
{
    int i, tmp;
    int test_cnt, train_cnt;
    float res;

    test_cnt = 0;
    for(i=0;i<ds->num_test_ex;i++){
        feed_input(i, nn, ds->test_set);
        forward_prop(nn);
        tmp = (int)round(nn->lay[nn->num_lay-1].neu[nn->lay[nn->num_lay-1].num_neu-1].actv);
        if(tmp == (int)ds->test_set[i][nn->lay[0].num_neu+nn->lay[nn->num_lay-1].num_neu-1])
            test_cnt++;
    }
    res = (float)test_cnt / ds->num_test_ex;
    printf("Test set accuracy rate: %.2f%%\n", res *100);

    train_cnt = 0;
    for(i=0;i<ds->num_training_ex;i++){
        feed_input(i, nn, ds->train_set);
        forward_prop(nn);
        tmp = (int)round(nn->lay[nn->num_lay-1].neu[nn->lay[nn->num_lay-1].num_neu-1].actv);
        if(tmp == (int)ds->train_set[i][nn->lay[0].num_neu+nn->lay[nn->num_lay-1].num_neu-1])
            train_cnt++;
    }
    res = (float)train_cnt / ds->num_training_ex;
    printf("Training set accuracy rate: %.2f%%\n", res *100);

    res = (float)(test_cnt+train_cnt) / (ds->num_test_ex + ds->num_training_ex);
    printf("Overall accuracy rate: %.2f%%\n", res *100);

    return;
}

// TODO: Add different Activation functions
//void activation_functions()

// TEST pirnt all the weights of Neural Network
void print_weights(neural_network *nn)
{
    int i, j, k;
    for(i=0; i<nn->num_lay-1;i++){
        printf("Layer %d :\n", i);
        for(j=0;j<nn->lay[i].num_neu;j++){
            printf("Neuron %d weights :", j);
            for(k=0;k<nn->lay[i+1].num_neu;k++){
                printf("%.2f ", nn->lay[i].neu[j].weights[k]);
            }
            printf("\n");
        }
    }
}

// TODO: Print outputs of all the neurals
void print_actvs(neural_network *nn)
{

}

void destroy_dataset(data_set *this)
{
    int i;
    for(i=0;i<this->num_training_ex;i++){
        free(this->train_set[i]);
        this->train_set[i] = NULL;
    }
    free(this->train_set);
    this->train_set = NULL;
    for(i=0;i<this->num_test_ex;i++){
        free(this->test_set[i]);
        this->test_set[i] = NULL;
    }
    free(this->test_set);
    this->test_set = NULL;

    return;
}

void destroy_network(neural_network *this)
{
    int i;
    free(this->cost);
    this->cost = NULL;
    // Free separately by destroy_hyperparameter
    this->hp = NULL;
    for(i=0;i<this->num_lay - 1;i++){
        destroy_layer(&(this->lay[i]));
    }
    // last layer has no neuron malloced
    free(this->lay[i].neu);
    this->lay[i].neu = NULL;
    free(this->lay);
    this->lay = NULL;

    return;
}

void destroy_hyperparameter(hyper_parameter *this)
{
    free(this->num_neurons);
    this->num_neurons = NULL;

    return;
}
