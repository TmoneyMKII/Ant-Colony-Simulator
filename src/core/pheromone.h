/**
 * @file pheromone.h
 * @brief Grid-based pheromone fields
 *
 * FOOD   (green): laid by returning ants, leads TO food
 * HOME   (blue):  laid by foraging ants, leads TO the nest
 * DANGER (red):   laid where ants die
 */

#ifndef PHEROMONE_H
#define PHEROMONE_H

#include "types.h"

typedef struct {
    int cell_size;
    int cols;
    int rows;
    float *layers[PHEROMONE_TYPE_COUNT];   /**< cols*rows each, row-major */
} PheromoneMap;

bool pheromone_init(PheromoneMap *map, int world_width, int world_height, int cell_size);
void pheromone_free(PheromoneMap *map);
void pheromone_clear(PheromoneMap *map);

/** @brief Decay every cell: trails by trail_keep, danger by danger_keep */
void pheromone_evaporate(PheromoneMap *map, float trail_keep, float danger_keep);

/** @brief Add pheromone at a world position (clamped to PHEROMONE_MAX_VALUE) */
void pheromone_deposit(PheromoneMap *map, PheromoneType type, float x, float y, float amount);

/** @brief Strength at a world position (0 outside the map) */
float pheromone_sample(const PheromoneMap *map, PheromoneType type, float x, float y);

/**
 * @brief Direction toward the strongest neighbouring cell
 *
 * Neighbours behind the ant (> 90 degrees from heading) are down-weighted
 * so ants don't turn back along the trail they just laid.
 * @return true if any neighbour is above the detection threshold
 */
bool pheromone_steer(const PheromoneMap *map, PheromoneType type,
                     float x, float y, float heading, float *out_dir);

/** @brief Raw layer data for rendering */
static inline const float *pheromone_layer(const PheromoneMap *map, PheromoneType type) {
    return map->layers[type];
}

#endif /* PHEROMONE_H */
