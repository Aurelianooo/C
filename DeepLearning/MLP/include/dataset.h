#ifndef DATASET_H
#define DATASET_H

#include "common.h"

extern char *strdup(const char *);

typedef struct
{
    const char *dataset_path;
    int num_train;
    int num_test;
    int num_features;
    int num_labels;
} DatasetParams;

typedef struct Dataset
{
    char *dataset_path;
    int num_train;
    int num_test;
    int num_features;
    int num_labels;
    float **train_set;
    float **test_set;

    int (*load)(struct Dataset *self);
    void (*shuffle)(struct Dataset *self);
    void (*destroy)(struct Dataset *self);
} Dataset;

// CTOR
Dataset *dataset_create(DatasetParams *params);

#endif
