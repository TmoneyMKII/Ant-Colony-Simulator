/**
 * @file ant.h
 * @brief Individual ant agent
 *
 * State machine:
 *   FORAGING  - searching for food; lays HOME trail, follows FOOD trail
 *   RETURNING - carrying food; lays FOOD trail, follows HOME trail
 *
 * Ants never touch colony-level state directly: ant_update() reports
 * deliveries and deaths through its return value and the world applies them.
 */

#ifndef ANT_H
#define ANT_H

#include "types.h"
#include "neural_net.h"
#include "vision.h"

struct World;

typedef struct {
    uint32_t id;            /**< Unique for the lifetime of the world; new id on respawn */

    /* Motion */
    Vec2 pos;
    float heading;          /**< Radians */
    float speed;            /**< Current speed, pixels per tick */

    /* State */
    AntState state;
    float energy;
    bool knows_food;
    Vec2 last_food;
    int deposit_timer;

    /* Stuck detection */
    int stuck_timer;
    Vec2 checkpoint;
    int stuck_strikes;

    /* Lifetime stats */
    int age;                /**< Ticks alive */
    int deliveries;

    /* Current-generation stats (fitness is measured per generation) */
    int gen_age;
    int gen_deliveries;

    /* Brain */
    Genome genome;
    NNActivations nn;       /**< Last forward pass, for the inspector */
    VisionRay vision[NN_NUM_VISION_RAYS];
} Ant;

typedef enum {
    ANT_OK = 0,
    ANT_DELIVERED,          /**< Dropped one unit of food at the nest this tick */
    ANT_DIED_STARVED,
    ANT_DIED_STUCK
} AntOutcome;

/** @brief Place a fresh ant at pos with the given brain */
void ant_init(Ant *ant, uint32_t id, Vec2 pos, const Genome *genome, Rng *rng);

/** @brief Advance one tick. May pick food up from the world; never edits the nest. */
AntOutcome ant_update(Ant *ant, int self_index, struct World *w);

/** @brief Fitness over the current generation */
float ant_fitness(const Ant *ant);

#endif /* ANT_H */
