/**
 * @file ant.h
 * @brief Ant agent behavior and state machine
 */

#ifndef ANT_H
#define ANT_H

#include "types.h"

/**
 * @brief Initialize a new ant at position
 * @param ant Ant to initialize
 * @param x Starting X position
 * @param y Starting Y position
 * @param id Unique ID for this ant
 * @param colony Reference to colony
 */
void ant_init(Ant *ant, float x, float y, uint32_t id, Colony *colony);

/**
 * @brief Free ant resources (neural network)
 */
void ant_cleanup(Ant *ant);

/**
 * @brief Update ant behavior for one frame
 * @param ant Ant to update
 * @param pheromone_map Pheromone system
 * @param food_sources Food source array
 * @param food_count Number of food sources
 * @param all_ants All ants for collision/vision
 * @param ant_count Number of ants
 * @param wall_manager Wall collision system
 * @param bounds Simulation bounds
 * @return false if ant died this frame
 */
bool ant_update(Ant *ant,
                PheromoneMap *pheromone_map,
                FoodSource *food_sources, int food_count,
                Ant *all_ants, int ant_count,
                const WallManager *wall_manager,
                const Rect *bounds);

/**
 * @brief Force ant into specific state
 */
void ant_set_state(Ant *ant, AntState state);

/**
 * @brief Calculate fitness score for this ant
 */
float ant_calculate_fitness(const Ant *ant);

/**
 * @brief Get neural network inputs for this ant
 * @param ant Ant to query
 * @param inputs Output array (must have NN_INPUT_SIZE elements)
 * @param pheromone_map Pheromone system for sensing
 */
void ant_get_nn_inputs(Ant *ant, float *inputs, const PheromoneMap *pheromone_map);

/**
 * @brief Apply neural network outputs to ant movement
 * @param ant Ant to update
 * @param outputs Neural network outputs (NN_OUTPUT_SIZE elements)
 */
void ant_apply_nn_outputs(Ant *ant, const float *outputs);

#endif /* ANT_H */
