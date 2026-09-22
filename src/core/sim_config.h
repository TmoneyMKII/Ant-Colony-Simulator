/**
 * @file sim_config.h
 * @brief Runtime-tunable simulation parameters
 *
 * Every field can be changed while the simulation runs (the UI exposes
 * them as sliders) except those marked "on reset", which only take
 * effect the next time the world is reset.
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

typedef enum {
    BRAIN_NEURAL = 0,   /**< Evolved neural network steers the ant */
    BRAIN_CLASSIC = 1   /**< Hand-written pheromone following */
} BrainMode;

typedef struct {
    /* World (on reset) */
    int   world_width;
    int   world_height;
    int   food_source_count;
    float food_min;             /**< Food units per spawned source (min) */
    float food_max;             /**< Food units per spawned source (max) */

    /* Population */
    int   population;           /**< Target ant count */

    /* Ants */
    BrainMode brain_mode;
    float ant_speed;            /**< Base speed, pixels per tick */
    float turn_rate;            /**< Max turn per tick (radians) at full NN output */
    float energy_drain;         /**< Energy lost per tick */
    float delivery_energy;      /**< Energy restored per delivery */
    float trail_sensitivity;    /**< Classic brain: blend toward trail direction (0-1) */

    /* Pheromones */
    float deposit_amount;       /**< Pheromone laid per deposit */
    int   deposit_interval;     /**< Ticks between deposits */
    float trail_evaporation;    /**< Fraction of food/home trail kept each tick */
    float danger_evaporation;   /**< Fraction of danger kept each tick */
    float danger_on_death;      /**< Danger pheromone laid where an ant dies */

    /* Evolution */
    int   generation_ticks;     /**< Length of one generation */
    int   elite_count;          /**< Genomes kept as breeding parents (<= ELITE_MAX) */
    float mutation_rate;        /**< Chance each weight mutates */
    float mutation_strength;    /**< Std-dev of weight mutations */
    float replace_fraction;     /**< Weakest living ants replaced each generation */
} SimConfig;

/** @brief Fill cfg with the default tuning */
void sim_config_defaults(SimConfig *cfg);

/** @brief Clamp every field to its valid range */
void sim_config_sanitize(SimConfig *cfg);

#endif /* SIM_CONFIG_H */
