#include "dataset.h"

static void shuffle_2d_array(float **array, int rows, int cols)
{
    float *temp = (float *)malloc(cols * sizeof(float));
    for (int i = 0; i < rows - 1; i++)
    {
        int idx = i + rand() % (rows - i);
        memcpy(temp, array[i], cols * sizeof(float));
        memcpy(array[i], array[idx], cols * sizeof(float));
        memcpy(array[idx], temp, cols * sizeof(float));
    }
    free(temp);
}

static int dataset_load(Dataset *self)
{
    int total_samples = self->num_train + self->num_test;
    int cols = self->num_features + self->num_labels;

    float **all_data = (float **)malloc(total_samples * sizeof(float *));
    for (int i = 0; i < total_samples; i++)
    {
        all_data[i] = (float *)malloc(cols * sizeof(float));
    }

    FILE *fptr = fopen(self->dataset_path, "r");
    if (!fptr)
    {
        fprintf(stderr, "Failed to open dataset file: %s\n", self->dataset_path);
        for (int i = 0; i < total_samples; i++)
            free(all_data[i]);
        free(all_data);
        return ERR_FILE_OPEN;
    }

    char buffer[512];
    int i = 0;
    while (fgets(buffer, sizeof(buffer), fptr) != NULL && i < total_samples)
    {
        char *token = strtok(buffer, ",");
        int j = 0;
        while (token && j < cols)
        {
            all_data[i][j] = atof(token);
            token = strtok(NULL, ",");
            j++;
        }
        i++;
    }
    fclose(fptr);

    shuffle_2d_array(all_data, total_samples, cols);

    // Divide train set and test set
    for (i = 0; i < self->num_train; i++)
    {
        memcpy(self->train_set[i], all_data[i], cols * sizeof(float));
    }
    for (; i < total_samples; i++)
    {
        memcpy(self->test_set[i - self->num_train], all_data[i], cols * sizeof(float));
    }

    for (i = 0; i < total_samples; i++)
    {
        free(all_data[i]);
    }
    free(all_data);

    return SUCCESS;
}

static void dataset_shuffle(Dataset *self)
{
    int cols = self->num_features + self->num_labels;
    shuffle_2d_array(self->train_set, self->num_train, cols);
}

static void dataset_destroy(Dataset *self)
{
    if (!self)
        return;

    if (self->train_set)
    {
        for (int i = 0; i < self->num_train; i++)
        {
            free(self->train_set[i]);
        }
        free(self->train_set);
    }

    if (self->test_set)
    {
        for (int i = 0; i < self->num_test; i++)
        {
            free(self->test_set[i]);
        }
        free(self->test_set);
    }

    if (self->dataset_path)
    {
        free(self->dataset_path);
    }

    free(self);
}

Dataset *dataset_create(DatasetParams *params)
{
    if (!params || !params->dataset_path || params->num_train <= 0 || params->num_test <= 0)
    {
        fprintf(stderr, "Invalid parameters for Dataset creation\n");
        return NULL;
    }

    Dataset *self = (Dataset *)malloc(sizeof(Dataset));
    if (!self)
    {
        fprintf(stderr, "Memory allocation failed for Dataset\n");
        return NULL;
    }

    self->dataset_path = strdup(params->dataset_path);
    self->num_train = params->num_train;
    self->num_test = params->num_test;
    self->num_features = params->num_features;
    self->num_labels = params->num_labels;

    int cols = self->num_features + self->num_labels;
    self->train_set = (float **)malloc(self->num_train * sizeof(float *));
    self->test_set = (float **)malloc(self->num_test * sizeof(float *));

    if (!self->train_set || !self->test_set || !self->dataset_path)
    {
        fprintf(stderr, "Memory allocation failed for [Dataset arrays]\n");
        dataset_destroy(self);
        return NULL;
    }

    for (int i = 0; i < self->num_train; i++)
    {
        self->train_set[i] = (float *)malloc(cols * sizeof(float));
        if (!self->train_set[i])
        {
            fprintf(stderr, "Memory allocation failed for [train set row] %d\n", i);
            dataset_destroy(self);
            return NULL;
        }
    }

    for (int i = 0; i < self->num_test; i++)
    {
        self->test_set[i] = (float *)malloc(cols * sizeof(float));
        if (!self->test_set[i])
        {
            fprintf(stderr, "Memory allocation failed for [test set row] %d\n", i);
            dataset_destroy(self);
            return NULL;
        }
    }

    self->load = dataset_load;
    self->shuffle = dataset_shuffle;
    self->destroy = dataset_destroy;

    return self;
}
