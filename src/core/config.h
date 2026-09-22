/**
 * @file config.h
 * @brief Compile-time constants for the ant colony simulator
 *
 * Only structural values live here (array sizes, network shape, geometry).
 * Anything a user might want to tune while the simulation runs lives in
 * SimConfig (sim_config.h) instead.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ============== Simulation clock ============== */
#define SIM_TICK_RATE           60      /* simulation ticks per simulated second */

/* ============== World defaults ============== */
#define DEFAULT_WORLD_WIDTH     1920
#define DEFAULT_WORLD_HEIGHT    1080
#define MAX_POPULATION          2000

/* ============== Geometry ============== */
#define ANT_RADIUS              6.0f
#define NEST_RADIUS             20.0f
#define FOOD_RADIUS             10.0f
#define FOOD_PICKUP_RANGE       15.0f
#define NEST_DROPOFF_RANGE      25.0f
#define ANT_SMELL_RANGE         150.0f  /* classic brain: food attraction radius */
#define ANT_MAX_ENERGY          100.0f

/* ============== Stuck detection ============== */
#define STUCK_CHECK_INTERVAL    180     /* ticks between progress checks */
#define STUCK_MIN_MOVEMENT      80.0f   /* must move this far per interval */
#define STUCK_MAX_STRIKES       5       /* failed checks before the ant dies */

/* ============== Food ============== */
#define FOOD_RESPAWN_INTERVAL   600     /* ticks between depleted-source respawns */
#define FOOD_EDGE_MARGIN        50.0f

/* ============== Death markers ============== */
#define DEATH_MARKER_DURATION   600     /* ticks */
#define MAX_DEATH_MARKERS       512

/* ============== Pheromones ============== */
#define PHEROMONE_CELL_SIZE         20
#define PHEROMONE_MAX_VALUE         200.0f
#define PHEROMONE_DETECT_THRESHOLD  10.0f

/* ============== Spatial partitioning ============== */
#ifndef ANT_GRID_CELL_SIZE
#define ANT_GRID_CELL_SIZE      50      /* cells the vision query sweeps around each ant */
#endif
#define WALL_GRID_CELL_SIZE     64

/* ============== Vision ============== */
#define NN_NUM_VISION_RAYS      7
#define VISION_FOV_DEGREES      180.0f
#define VISION_RAY_LENGTH       100.0f

/* ============== Neural network ============== */
#define NN_VISION_INPUTS        (NN_NUM_VISION_RAYS * 3)  /* wall, ant, food per ray */
#define NN_STATE_INPUTS         6
#define NN_INPUT_SIZE           (NN_VISION_INPUTS + NN_STATE_INPUTS)  /* 27 */
#define NN_HIDDEN_SIZE          16
#define NN_OUTPUT_SIZE          3

/* ============== Evolution ============== */
#define ELITE_MAX               32
#define FITNESS_PER_DELIVERY    15.0f
#define FITNESS_PER_TICK        0.01f
#define ELITE_CARRYOVER_DECAY   0.8f    /* previous elites re-enter the pool at this fitness scale */

/* ============== Statistics history ============== */
#define STATS_SAMPLE_INTERVAL   30      /* ticks between samples (0.5 s) */
#define STATS_HISTORY_LEN       600     /* samples kept (5 min of sim time) */
#define GEN_HISTORY_LEN         256     /* generations kept */

#endif /* CONFIG_H */
