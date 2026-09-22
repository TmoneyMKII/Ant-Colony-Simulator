/**
 * @file neural_net.c
 * @brief Forward pass and genetic operators for the ant brain
 */

#include "neural_net.h"
#include "utils.h"
#include <string.h>

void nn_randomize(Genome *g, float scale, Rng *rng) {
    for (int i = 0; i < NN_WEIGHT_COUNT; i++) {
        g->w[i] = rng_gaussian(rng, 0.0f, scale);
    }
}

void nn_forward(const Genome *g, const float inputs[NN_INPUT_SIZE], NNActivations *act) {
    float hidden[NN_HIDDEN_SIZE];

    memcpy(act->inputs, inputs, sizeof(act->inputs));

    /* h = tanh(b_h + sum_i in[i] * W_ih[i]) -- inner loop is contiguous */
    memcpy(hidden, &g->w[NN_OFFSET_BH], sizeof(hidden));
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        const float x = inputs[i];
        const float *row = &g->w[NN_OFFSET_IH + i * NN_HIDDEN_SIZE];
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            hidden[j] += x * row[j];
        }
    }
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        hidden[j] = tanhf(hidden[j]);
    }
    memcpy(act->hidden, hidden, sizeof(hidden));

    /* o = tanh(b_o + sum_j h[j] * W_ho[j]) */
    float out[NN_OUTPUT_SIZE];
    memcpy(out, &g->w[NN_OFFSET_BO], sizeof(out));
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        const float h = hidden[j];
        const float *row = &g->w[NN_OFFSET_HO + j * NN_OUTPUT_SIZE];
        for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
            out[k] += h * row[k];
        }
    }
    for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
        act->outputs[k] = tanhf(out[k]);
    }
}

void nn_crossover(Genome *child, const Genome *a, const Genome *b, Rng *rng) {
    for (int i = 0; i < NN_WEIGHT_COUNT; i++) {
        child->w[i] = (rng_float(rng) < 0.5f) ? a->w[i] : b->w[i];
    }
}

void nn_mutate(Genome *g, float rate, float strength, Rng *rng) {
    for (int i = 0; i < NN_WEIGHT_COUNT; i++) {
        if (rng_float(rng) < rate) {
            g->w[i] += rng_gaussian(rng, 0.0f, strength);
        }
    }
}

const char *nn_input_label(int i) {
    static const char *state_labels[NN_STATE_INPUTS] = {
        "Food trail", "Home trail", "Nest dist", "Nest bearing", "Carrying", "Energy"
    };
    static const char *ray_labels[3][NN_NUM_VISION_RAYS] = {
        {"Wall 1", "Wall 2", "Wall 3", "Wall 4", "Wall 5", "Wall 6", "Wall 7"},
        {"Ant 1",  "Ant 2",  "Ant 3",  "Ant 4",  "Ant 5",  "Ant 6",  "Ant 7"},
        {"Food 1", "Food 2", "Food 3", "Food 4", "Food 5", "Food 6", "Food 7"},
    };
    if (i < 0 || i >= NN_INPUT_SIZE) return "?";
    if (i < NN_VISION_INPUTS) return ray_labels[i / NN_NUM_VISION_RAYS][i % NN_NUM_VISION_RAYS];
    return state_labels[i - NN_VISION_INPUTS];
}

const char *nn_output_label(int k) {
    static const char *labels[NN_OUTPUT_SIZE] = {"Turn", "Speed", "Explore"};
    return (k >= 0 && k < NN_OUTPUT_SIZE) ? labels[k] : "?";
}
