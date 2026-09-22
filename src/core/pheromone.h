/**
 * @file pheromone.h
 * @brief Pheromone trail system
 */

#ifndef PHEROMONE_H
#define PHEROMONE_H

#include "types.h"

/**
 * @brief Create a new pheromone map
 * @param width World width in pixels
 * @param height World height in pixels
 * @param cell_size Grid cell size
 * @return Pointer to new map, or NULL on failure
 */
PheromoneMap* pheromone_create(int width, int height, int cell_size);

/**
 * @brief Free pheromone map memory
 */
void pheromone_destroy(PheromoneMap *map);

/**
 * @brief Clear all pheromones
 */
void pheromone_clear(PheromoneMap *map);

/**
 * @brief Update pheromones (apply evaporation)
 */
void pheromone_update(PheromoneMap *map);

/**
 * @brief Deposit pheromone at world position
 * @param map Pheromone map
 * @param x World x coordinate
 * @param y World y coordinate
 * @param amount Amount to deposit
 * @param type Type of pheromone
 */
void pheromone_deposit(PheromoneMap *map, float x, float y, float amount, PheromoneType type);

/* Convenience functions */
static inline void pheromone_deposit_food(PheromoneMap *map, float x, float y, float amount) {
    pheromone_deposit(map, x, y, amount, PHEROMONE_FOOD_TRAIL);
}

static inline void pheromone_deposit_home(PheromoneMap *map, float x, float y, float amount) {
    pheromone_deposit(map, x, y, amount, PHEROMONE_HOME_TRAIL);
}

static inline void pheromone_deposit_danger(PheromoneMap *map, float x, float y, float amount) {
    pheromone_deposit(map, x, y, amount, PHEROMONE_DANGER);
}

/**
 * @brief Get pheromone strength at world position
 */
float pheromone_get_strength(const PheromoneMap *map, float x, float y, PheromoneType type);

/**
 * @brief Get direction to follow pheromone trail
 * @param map Pheromone map
 * @param x World x coordinate
 * @param y World y coordinate
 * @param type Type of pheromone
 * @param current_dir Current heading (for forward bias), or NAN to ignore
 * @param out_dir Output: direction in radians
 * @return true if trail found, false otherwise
 */
bool pheromone_get_direction(const PheromoneMap *map, float x, float y, 
                              PheromoneType type, float current_dir, float *out_dir);

/**
 * @brief Get danger level at position
 */
float pheromone_get_danger(const PheromoneMap *map, float x, float y);

/**
 * @brief Convert world coordinates to grid coordinates
 */
void pheromone_to_grid(const PheromoneMap *map, float x, float y, int *gx, int *gy);

/**
 * @brief Get grid dimensions
 */
void pheromone_get_grid_size(const PheromoneMap *map, int *width, int *height);

/**
 * @brief Get raw pheromone value at grid cell
 */
float pheromone_get_cell(const PheromoneMap *map, int gx, int gy, PheromoneType type);

#endif /* PHEROMONE_H */
