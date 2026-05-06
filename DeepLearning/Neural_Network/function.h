#ifndef FUNCTION_H
#define FUNCTION_H

#include <stdlib.h>
#include <math.h>

/* ------- LOSS ------- */

float mean_square_error(float, float);
float binary_cross_entropy(float, float);

/* ------- ACTIVATION ------- */

float relu(float);
float sigmoid(float);

/* ------- NORMALIZATION ------- */

float kaiming_normal(int);

#endif
