/**
 * @file neural_net.c
 * @brief Neural network implementation with optimized forward pass
 * 
 * Feedforward neural network for ant decision-making:
 *   Input (27) -> Hidden (16, tanh) -> Output (3)
 * 
 * Input layout:
 *   [0-6]:   Wall distances per ray (7 rays)
 *   [7-13]:  Ant distances per ray
 *   [14-20]: Food distances per ray
 *   [21]:    Food pheromone strength
 *   [22]:    Home pheromone strength
 *   [23]:    Distance to colony (normalized)
 *   [24]:    Direction to colony (normalized)
 *   [25]:    Carrying food (0 or 1)
 *   [26]:    Energy level (0 to 1)
 * 
 * Output:
 *   [0]: Turn amount (-1 to 1)
 *   [1]: Speed modifier (0.5 to 1.5)
 *   [2]: Exploration tendency (0 to 1)
 */

#include "neural_net.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* Calculate total weight count */
#define WEIGHTS_IH (NN_INPUT_SIZE * NN_HIDDEN_SIZE)
#define WEIGHTS_HO (NN_HIDDEN_SIZE * NN_OUTPUT_SIZE)
#define BIAS_H     NN_HIDDEN_SIZE
#define BIAS_O     NN_OUTPUT_SIZE
#define TOTAL_WEIGHTS (WEIGHTS_IH + WEIGHTS_HO + BIAS_H + BIAS_O)

int nn_get_weight_count(void) {
    return TOTAL_WEIGHTS;
}

/**
 * @brief Unpack flat weights into matrices for fast access
 */
static void nn_unpack_weights(NeuralNetwork *nn) {
    const float *w = nn->weights;
    int idx = 0;
    
    /* Input -> Hidden weights */
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            nn->w_ih[i][j] = w[idx++];
        }
    }
    
    /* Hidden -> Output weights */
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        for (int j = 0; j < NN_OUTPUT_SIZE; j++) {
            nn->w_ho[i][j] = w[idx++];
        }
    }
    
    /* Hidden biases */
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        nn->b_h[i] = w[idx++];
    }
    
    /* Output biases */
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        nn->b_o[i] = w[idx++];
    }
}

NeuralNetwork* nn_create(void) {
    NeuralNetwork *nn = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
    if (!nn) return NULL;
    
    nn->total_weights = TOTAL_WEIGHTS;
    nn->weights = (float*)malloc(sizeof(float) * TOTAL_WEIGHTS);
    if (!nn->weights) {
        free(nn);
        return NULL;
    }
    
    /* Xavier initialization */
    float scale = 0.5f;
    for (int i = 0; i < TOTAL_WEIGHTS; i++) {
        nn->weights[i] = rand_gaussian(0.0f, scale);
    }
    
    nn_unpack_weights(nn);
    
    /* Initialize cached activations to zero */
    memset(nn->last_inputs, 0, sizeof(nn->last_inputs));
    memset(nn->last_hidden, 0, sizeof(nn->last_hidden));
    memset(nn->last_outputs, 0, sizeof(nn->last_outputs));
    
    return nn;
}

NeuralNetwork* nn_create_with_weights(const float *weights, int count) {
    if (count != TOTAL_WEIGHTS) return NULL;
    
    NeuralNetwork *nn = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
    if (!nn) return NULL;
    
    nn->total_weights = TOTAL_WEIGHTS;
    nn->weights = (float*)malloc(sizeof(float) * TOTAL_WEIGHTS);
    if (!nn->weights) {
        free(nn);
        return NULL;
    }
    
    memcpy(nn->weights, weights, sizeof(float) * TOTAL_WEIGHTS);
    nn_unpack_weights(nn);
    
    memset(nn->last_inputs, 0, sizeof(nn->last_inputs));
    memset(nn->last_hidden, 0, sizeof(nn->last_hidden));
    memset(nn->last_outputs, 0, sizeof(nn->last_outputs));
    
    return nn;
}

void nn_destroy(NeuralNetwork *nn) {
    if (nn) {
        free(nn->weights);
        free(nn);
    }
}

NeuralNetwork* nn_copy(const NeuralNetwork *src) {
    if (!src) return NULL;
    return nn_create_with_weights(src->weights, src->total_weights);
}

/**
 * @brief Forward pass with cache-friendly memory access
 */
void nn_forward(NeuralNetwork *nn, const float *inputs, float *outputs) {
    float hidden[NN_HIDDEN_SIZE];
    
    /* Store inputs for visualization */
    memcpy(nn->last_inputs, inputs, sizeof(float) * NN_INPUT_SIZE);
    
    /* Hidden layer: h = tanh(W_ih * inputs + b_h) */
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        float sum = nn->b_h[j];
        
        /* Unrolled for better vectorization */
        for (int i = 0; i < NN_INPUT_SIZE; i++) {
            sum += inputs[i] * nn->w_ih[i][j];
        }
        
        hidden[j] = fast_tanh(sum);
    }
    
    /* Store hidden for visualization */
    memcpy(nn->last_hidden, hidden, sizeof(float) * NN_HIDDEN_SIZE);
    
    /* Output layer: o = tanh(W_ho * hidden + b_o) */
    for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
        float sum = nn->b_o[k];
        
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            sum += hidden[j] * nn->w_ho[j][k];
        }
        
        outputs[k] = fast_tanh(sum);
    }
    
    /* Store outputs for visualization */
    memcpy(nn->last_outputs, outputs, sizeof(float) * NN_OUTPUT_SIZE);
}

void nn_get_weights(const NeuralNetwork *nn, float *weights) {
    memcpy(weights, nn->weights, sizeof(float) * nn->total_weights);
}

void nn_set_weights(NeuralNetwork *nn, const float *weights) {
    memcpy(nn->weights, weights, sizeof(float) * nn->total_weights);
    nn_unpack_weights(nn);
}

void nn_mutate(NeuralNetwork *nn, float mutation_rate, float mutation_strength) {
    for (int i = 0; i < nn->total_weights; i++) {
        if (randf() < mutation_rate) {
            nn->weights[i] += rand_gaussian(0.0f, mutation_strength);
        }
    }
    nn_unpack_weights(nn);
}

NeuralNetwork* nn_crossover(const NeuralNetwork *parent1, const NeuralNetwork *parent2) {
    NeuralNetwork *child = nn_create();
    if (!child) return NULL;
    
    /* Uniform crossover */
    for (int i = 0; i < TOTAL_WEIGHTS; i++) {
        child->weights[i] = (randf() < 0.5f) ? parent1->weights[i] : parent2->weights[i];
    }
    
    nn_unpack_weights(child);
    return child;
}
