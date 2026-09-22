/**
 * @file neural_net.h
 * @brief Fixed-size feedforward network used as each ant's brain
 *
 * Architecture: 27 inputs -> 16 hidden (tanh) -> 3 outputs (tanh)
 *
 * Input layout:
 *   [0-6]    wall closeness per vision ray (1 = touching, 0 = nothing in range)
 *   [7-13]   ant closeness per ray
 *   [14-20]  food closeness per ray
 *   [21]     food pheromone ahead
 *   [22]     home pheromone ahead
 *   [23]     distance to nest (normalised)
 *   [24]     bearing to nest relative to heading (-1..1)
 *   [25]     carrying food (0/1)
 *   [26]     energy (0..1)
 *
 * Outputs:
 *   [0] turn (-1..1)   [1] speed (-1..1)   [2] explore (> 0.5 adds jitter)
 *
 * Weights are stored flat in a Genome so they can be copied, crossed over
 * and mutated without any allocation:
 *   [input->hidden  I*H, row-major by input]
 *   [hidden->output H*O, row-major by hidden]
 *   [hidden biases  H]
 *   [output biases  O]
 */

#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "config.h"
#include "utils.h"

#define NN_OFFSET_IH   0
#define NN_OFFSET_HO   (NN_OFFSET_IH + NN_INPUT_SIZE * NN_HIDDEN_SIZE)
#define NN_OFFSET_BH   (NN_OFFSET_HO + NN_HIDDEN_SIZE * NN_OUTPUT_SIZE)
#define NN_OFFSET_BO   (NN_OFFSET_BH + NN_HIDDEN_SIZE)
#define NN_WEIGHT_COUNT (NN_OFFSET_BO + NN_OUTPUT_SIZE)

/** @brief All weights and biases of one network */
typedef struct {
    float w[NN_WEIGHT_COUNT];
} Genome;

/** @brief Activations from the last forward pass (kept for visualisation) */
typedef struct {
    float inputs[NN_INPUT_SIZE];
    float hidden[NN_HIDDEN_SIZE];
    float outputs[NN_OUTPUT_SIZE];
} NNActivations;

/** @brief Random weights drawn from N(0, scale) */
void nn_randomize(Genome *g, float scale, Rng *rng);

/** @brief Run the network; results land in act->outputs */
void nn_forward(const Genome *g, const float inputs[NN_INPUT_SIZE], NNActivations *act);

/** @brief Uniform crossover: each weight comes from a or b with equal chance */
void nn_crossover(Genome *child, const Genome *a, const Genome *b, Rng *rng);

/** @brief Add N(0, strength) noise to each weight with probability rate */
void nn_mutate(Genome *g, float rate, float strength, Rng *rng);

/** @brief Weight from input i to hidden j */
static inline float nn_weight_ih(const Genome *g, int i, int j) {
    return g->w[NN_OFFSET_IH + i * NN_HIDDEN_SIZE + j];
}

/** @brief Weight from hidden j to output k */
static inline float nn_weight_ho(const Genome *g, int j, int k) {
    return g->w[NN_OFFSET_HO + j * NN_OUTPUT_SIZE + k];
}

/** @brief Short human-readable label for an input neuron */
const char *nn_input_label(int i);

/** @brief Short human-readable label for an output neuron */
const char *nn_output_label(int k);

#endif /* NEURAL_NET_H */
