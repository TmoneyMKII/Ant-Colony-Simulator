/**
 * @file colony.h
 * @brief Colony management and spawning
 */

#ifndef COLONY_H
#define COLONY_H

#include "types.h"

/**
 * @brief Create a new colony
 * @param x Center X position
 * @param y Center Y position
 * @param width Simulation width
 * @param height Simulation height
 * @param bounds Simulation bounds rectangle
 * @return Pointer to new colony, or NULL on failure
 */
Colony* colony_create(float x, float y, int width, int height, Rect bounds);

/**
 * @brief Free colony and all resources
 */
void colony_destroy(Colony *colony);

/**
 * @brief Update colony for one frame
 */
void colony_update(Colony *colony);

/**
 * @brief Spawn a new ant
 * @return true if ant was spawned
 */
bool colony_spawn_ant(Colony *colony);

/**
 * @brief Add food to colony storage
 */
void colony_add_food(Colony *colony, float amount);

/**
 * @brief Add a food source at position
 * @return true if added successfully
 */
bool colony_add_food_source(Colony *colony, float x, float y, float amount);

/**
 * @brief Add a death marker at position
 */
void colony_add_death_marker(Colony *colony, float x, float y);

/**
 * @brief Reset colony to initial state
 */
void colony_reset(Colony *colony);

/**
 * @brief Get statistics about the colony
 */
void colony_get_stats(const Colony *colony, 
                      int *out_population,
                      float *out_food_stored,
                      int *out_generation,
                      float *out_best_fitness);

#endif /* COLONY_H */
