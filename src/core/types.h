/**
 * @file types.h
 * @brief Small value types shared across the simulation core
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/** @brief 2D vector for positions and directions */
typedef struct {
    float x;
    float y;
} Vec2;

/** @brief Axis-aligned integer rectangle */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

/** @brief Ant behaviour states */
typedef enum {
    ANT_STATE_FORAGING = 0,   /**< Looking for food, follows food trail */
    ANT_STATE_RETURNING = 1   /**< Carrying food home, follows home trail */
} AntState;

/** @brief Types of pheromone trails */
typedef enum {
    PHEROMONE_FOOD = 0,       /**< Laid by returning ants, leads TO food */
    PHEROMONE_HOME = 1,       /**< Laid by foraging ants, leads TO the nest */
    PHEROMONE_DANGER = 2,     /**< Laid where ants die */
    PHEROMONE_TYPE_COUNT
} PheromoneType;

/** @brief Why an ant died */
typedef enum {
    DEATH_STARVED = 0,
    DEATH_STUCK = 1
} DeathCause;

#endif /* TYPES_H */
