/**
 * @file world.h
 * @brief The simulation: everything the renderer and UI read from
 *
 * The core links without SDL and has no notion of screens, colours or
 * input. A frontend creates a World, calls world_step() as often as it
 * likes, and reads the state back for display.
 */

#ifndef WORLD_H
#define WORLD_H

#include "types.h"
#include "sim_config.h"
#include "ant.h"
#include "evolution.h"
#include "pheromone.h"
#include "spatial_hash.h"
#include "walls.h"

/* ============== Entities ============== */

typedef struct {
    Vec2 pos;
    float amount;
    float max_amount;
} FoodSource;

typedef struct {
    Vec2 pos;
    float radius;
    float food_stored;      /**< Food delivered by ants and never consumed */
} Nest;

typedef struct {
    Vec2 pos;
    int ttl;                /**< Ticks until the marker disappears */
    DeathCause cause;
} DeathMarker;

/* ============== Events ============== */

typedef enum {
    SIM_EVENT_DELIVERY = 0,
    SIM_EVENT_DEATH,
    SIM_EVENT_GENERATION
} SimEventType;

/** @brief Something worth reacting to that happened during a step */
typedef struct {
    SimEventType type;
    uint32_t ant_id;
    Vec2 pos;
    int detail;             /**< DeathCause for deaths, generation number for generations */
} SimEvent;

/* ============== Statistics ============== */

/** @brief One periodic sample of colony-wide numbers, for graphs */
typedef struct {
    float food_stored;
    int alive;
    int carrying;
    float avg_energy;
    int deliveries;         /**< Deliveries during this sample window */
} StatsSample;

/** @brief Summary of one finished generation */
typedef struct {
    int generation;
    float best_fitness;
    float avg_fitness;
    int evaluated;
    int deliveries;
} GenerationStats;

typedef struct {
    uint64_t total_deliveries;
    uint64_t deaths_starved;
    uint64_t deaths_stuck;

    /* Live counts, refreshed every step */
    int carrying;
    float avg_energy;

    /* Rolling sample history (ring buffer, oldest first via stats_sample) */
    StatsSample samples[STATS_HISTORY_LEN];
    int sample_count;
    int sample_head;        /**< Index of the next write */
    int window_deliveries;

    /* Per-generation history */
    GenerationStats gens[GEN_HISTORY_LEN];
    int gen_count;
    int gen_head;
    int gen_deliveries;
} WorldStats;

/* ============== World ============== */

typedef struct World {
    SimConfig cfg;
    Rng rng;            /**< This world's random stream */
    uint32_t seed;
    int width;
    int height;
    uint64_t tick;

    Nest nest;

    Ant *ants;                  /**< Allocated to MAX_POPULATION */
    int ant_count;
    uint32_t next_ant_id;

    FoodSource *food;
    int food_count;
    int food_capacity;

    DeathMarker markers[MAX_DEATH_MARKERS];
    int marker_count;

    PheromoneMap pheromones;
    WallSet walls;
    Evolution evo;
    WorldStats stats;

    /* Broadphase over ant positions, rebuilt at the start of every step */
    SpatialHash ant_grid;
    Vec2 *ant_positions;

    /* Events produced by the most recent step */
    SimEvent *events;
    int event_count;
    int event_capacity;
} World;

/** @brief Create a world and populate it. cfg is copied. */
World *world_create(const SimConfig *cfg, uint32_t seed);

void world_destroy(World *w);

/**
 * @brief Rebuild the world from w->cfg with a new seed
 *
 * Walls, food, pheromones, ants and evolution all start over.
 */
void world_reset(World *w, uint32_t seed);

/** @brief Advance the simulation by one tick */
void world_step(World *w);

/** @brief Add a food source unless the position is inside a wall */
bool world_add_food(World *w, float x, float y, float amount);

/** @brief Regenerate the walls, keeping ants, food and brains */
void world_new_maze(World *w);

/** @brief Spawn or remove ants until the population matches cfg.population */
void world_apply_population(World *w);

/** @brief Index of the ant nearest (x, y) within radius, or -1 */
int world_ant_at(const World *w, float x, float y, float radius);

/** @brief Index of the ant with this id, or -1 if it is gone */
int world_ant_by_id(const World *w, uint32_t id);

/** @brief Take up to one unit of food from source index; returns what was taken */
float world_take_food(World *w, int food_index);

/** @brief Sample i of the history, 0 = oldest */
const StatsSample *stats_sample(const WorldStats *s, int i);

/** @brief Generation i of the history, 0 = oldest */
const GenerationStats *stats_generation(const WorldStats *s, int i);

#endif /* WORLD_H */
