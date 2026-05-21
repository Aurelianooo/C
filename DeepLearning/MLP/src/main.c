#include "network.h"
#include "dataset.h"
#include "optimizer.h"
#include "loss.h"
#include "common.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int main(void)
{
    srand((unsigned int)time(NULL));

    MLPParams mlp_params = {
        .num_layers = 4,
        // Dimensions: input 34, hidden 16&8, output 1
        .num_neurons = (int[]){34, 16, 8, 1},
        .initial_lr = 0.01f,
        .momentum = 0.9f,
        .epoch = 300,
        .lr_decay_epoch_ratio = 0.618f,
        .lr_decay_factor = 0.1f};

    DatasetParams ds_params = {
        .dataset_path = "../Datasets/ionosphere.data",
        .num_train = 320,
        .num_test = 31,
        .num_features = 34,
        .num_labels = 1 // Single-label binary classification
    };

    Loss *loss = create_loss(LOSS_BCE);
    if (!loss)
    {
        fprintf(stderr, "Failed to create loss function\n");
        return EXIT_FAILURE;
    }

    Optimizer *opt = create_optimizer(OPT_SGD_MOMENTUM, mlp_params.initial_lr,
                                      mlp_params.momentum, 0.0001f);
    if (!opt)
    {
        fprintf(stderr, "Failed to create optimizer\n");
        destroy_loss(loss);
        return EXIT_FAILURE;
    }

    MLP *mlp = mlp_create(&mlp_params, loss, opt);
    if (!mlp)
    {
        fprintf(stderr, "Failed to create MLP network\n");
        destroy_optimizer(opt);
        destroy_loss(loss);
        return EXIT_FAILURE;
    }
    printf("MLP network created successfully\n");

    Dataset *dataset = dataset_create(&ds_params);
    if (!dataset)
    {
        fprintf(stderr, "Failed to create dataset object\n");
        mlp->destroy(mlp);
        destroy_optimizer(opt);
        destroy_loss(loss);
        return EXIT_FAILURE;
    }

    if (dataset->load(dataset) != SUCCESS)
    {
        fprintf(stderr, "Failed to load dataset from %s\n", ds_params.dataset_path);
        dataset->destroy(dataset);
        mlp->destroy(mlp);
        destroy_optimizer(opt);
        destroy_loss(loss);
        return EXIT_FAILURE;
    }
    printf("Dataset loaded successfully (train: %d, test: %d)\n",
           dataset->num_train, dataset->num_test);

    printf("\nStarting training...\n");
    struct timeval epoch_start, epoch_end;
    int decay_epoch = (int)round(mlp_params.epoch * mlp_params.lr_decay_epoch_ratio);

    for (int epoch = 0; epoch < mlp_params.epoch; epoch++)
    {
        if (epoch == decay_epoch)
        {
            float new_lr = mlp_params.initial_lr * mlp_params.lr_decay_factor;
            opt->set_lr(opt, new_lr);
            printf("Learning rate decayed to %.6f at epoch %d\n", new_lr, epoch);
        }

        float total_loss = 0.0f;
        gettimeofday(&epoch_start, NULL);

        for (int i = 0; i < dataset->num_train; i++)
        {
            float *input = dataset->train_set[i];
            float *label = &dataset->train_set[i][ds_params.num_features];

            float sample_loss = mlp->train_step(mlp, input, label);
            total_loss += sample_loss;
        }

        gettimeofday(&epoch_end, NULL);
        long long epoch_time_ms = (epoch_end.tv_sec - epoch_start.tv_sec) * 1000LL +
                                  (epoch_end.tv_usec - epoch_start.tv_usec) / 1000LL;

        float avg_loss = total_loss / dataset->num_train;
        printf("Epoch [%3d/%d] | Avg Loss: %.6f | Time: %4lld ms\n",
               epoch + 1, mlp_params.epoch, avg_loss, epoch_time_ms);

        dataset->shuffle(dataset);
    }
    printf("Training completed!\n\n");

    printf("Evaluating model performance...\n");

    int test_correct = 0;
    for (int i = 0; i < dataset->num_test; i++)
    {
        float *input = dataset->test_set[i];
        float true_label = dataset->test_set[i][ds_params.num_features];
        float pred = mlp->predict(mlp, input);
        int pred_label = (int)round(pred);

        if (pred_label == (int)true_label)
        {
            test_correct++;
        }
    }
    float test_acc = (float)test_correct / dataset->num_test * 100.0f;

    int train_correct = 0;
    for (int i = 0; i < dataset->num_train; i++)
    {
        float *input = dataset->train_set[i];
        float true_label = dataset->train_set[i][ds_params.num_features];
        float pred = mlp->predict(mlp, input);
        int pred_label = (int)round(pred);

        if (pred_label == (int)true_label)
        {
            train_correct++;
        }
    }
    float train_acc = (float)train_correct / dataset->num_train * 100.0f;

    printf("Train Accuracy: %.2f%% (%d/%d)\n",
           train_acc, train_correct, dataset->num_train);
    printf("Test Accuracy:  %.2f%% (%d/%d)\n",
           test_acc, test_correct, dataset->num_test);
    printf("Overall Accuracy: %.2f%%\n",
           (float)(train_correct + test_correct) / (dataset->num_train + dataset->num_test) * 100.0f);

    dataset->destroy(dataset);
    mlp->destroy(mlp);
    destroy_optimizer(opt);
    destroy_loss(loss);

    printf("\nAll resources released successfully\n");
    return EXIT_SUCCESS;
}
