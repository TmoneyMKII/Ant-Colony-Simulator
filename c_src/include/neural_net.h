/**
 * @file neural_net.h
 * @brief Neural network for ant decision-making
 */

#ifndef NEURAL_NET_H
#define NEURAL_NET_H

#include "types.h"

/**
 * @brief Create a new neural network with random weights
 * @return Pointer to new network, or NULL on failure
 */
NeuralNetwork* nn_create(void);

/**
 * @brief Create a neural network with specific weights
 * @param weights Array of weights (copied)
 * @param count Number of weights
 * @return Pointer to new network, or NULL on failure
 */
NeuralNetwork* nn_create_with_weights(const float *weights, int count);

/**
 * @brief Free neural network memory
 */
void nn_destroy(NeuralNetwork *nn);

/**
 * @brief Copy neural network
 * @return New network with copied weights
 */
NeuralNetwork* nn_copy(const NeuralNetwork *src);

/**
 * @brief Forward pass through the network
 * @param nn Neural network
 * @param inputs Input array (must have NN_INPUT_SIZE elements)
 * @param outputs Output array (must have NN_OUTPUT_SIZE elements)
 */
void nn_forward(NeuralNetwork *nn, const float *inputs, float *outputs);

/**
 * @brief Get total number of weights in network
 */
int nn_get_weight_count(void);

/**
 * @brief Get network weights as flat array
 * @param nn Neural network
 * @param weights Output array (must have nn_get_weight_count() elements)
 */
void nn_get_weights(const NeuralNetwork *nn, float *weights);

/**
 * @brief Set network weights from flat array
 * @param nn Neural network
 * @param weights Input array (must have nn_get_weight_count() elements)
 */
void nn_set_weights(NeuralNetwork *nn, const float *weights);

/**
 * @brief Mutate network weights
 * @param nn Neural network
 * @param mutation_rate Probability of mutating each weight (0-1)
 * @param mutation_strength Standard deviation of mutation
 */
void nn_mutate(NeuralNetwork *nn, float mutation_rate, float mutation_strength);

/**
 * @brief Crossover two networks to create a child
 * @param parent1 First parent network
 * @param parent2 Second parent network
 * @return New child network
 */
NeuralNetwork* nn_crossover(const NeuralNetwork *parent1, const NeuralNetwork *parent2);

#endif /* NEURAL_NET_H */
