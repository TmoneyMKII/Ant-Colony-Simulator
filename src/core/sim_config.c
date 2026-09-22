/**
 * @file sim_config.c
 * @brief Default tuning for the simulation
 */

#include "sim_config.h"
#include "config.h"
#include "utils.h"

void sim_config_defaults(SimConfig *cfg) {
    cfg->world_width = DEFAULT_WORLD_WIDTH;
    cfg->world_height = DEFAULT_WORLD_HEIGHT;
    cfg->food_source_count = 12;
    cfg->food_min = 50.0f;
    cfg->food_max = 150.0f;

    cfg->population = 100;

    cfg->brain_mode = BRAIN_NEURAL;
    cfg->ant_speed = 2.0f;
    cfg->turn_rate = 0.3f;
    cfg->energy_drain = 0.01f;
    cfg->delivery_energy = 20.0f;
    cfg->trail_sensitivity = 0.15f;

    cfg->deposit_amount = 12.0f;
    cfg->deposit_interval = 3;
    cfg->trail_evaporation = 0.995f;
    cfg->danger_evaporation = 0.998f;
    cfg->danger_on_death = 150.0f;

    cfg->generation_ticks = 30 * SIM_TICK_RATE;
    cfg->elite_count = 10;
    cfg->mutation_rate = 0.1f;
    cfg->mutation_strength = 0.3f;
    cfg->replace_fraction = 0.2f;
}

void sim_config_sanitize(SimConfig *cfg) {
    cfg->world_width = clampi(cfg->world_width, 400, 8000);
    cfg->world_height = clampi(cfg->world_height, 300, 8000);
    cfg->food_source_count = clampi(cfg->food_source_count, 0, 200);
    cfg->food_min = clampf(cfg->food_min, 1.0f, 10000.0f);
    cfg->food_max = clampf(cfg->food_max, cfg->food_min, 10000.0f);

    cfg->population = clampi(cfg->population, 1, MAX_POPULATION);

    cfg->ant_speed = clampf(cfg->ant_speed, 0.1f, 10.0f);
    cfg->turn_rate = clampf(cfg->turn_rate, 0.0f, 1.5f);
    cfg->energy_drain = clampf(cfg->energy_drain, 0.0f, 1.0f);
    cfg->delivery_energy = clampf(cfg->delivery_energy, 0.0f, ANT_MAX_ENERGY);
    cfg->trail_sensitivity = clampf(cfg->trail_sensitivity, 0.0f, 1.0f);

    cfg->deposit_amount = clampf(cfg->deposit_amount, 0.0f, PHEROMONE_MAX_VALUE);
    cfg->deposit_interval = clampi(cfg->deposit_interval, 1, 120);
    cfg->trail_evaporation = clampf(cfg->trail_evaporation, 0.9f, 1.0f);
    cfg->danger_evaporation = clampf(cfg->danger_evaporation, 0.9f, 1.0f);
    cfg->danger_on_death = clampf(cfg->danger_on_death, 0.0f, PHEROMONE_MAX_VALUE);

    cfg->generation_ticks = clampi(cfg->generation_ticks, SIM_TICK_RATE, 600 * SIM_TICK_RATE);
    cfg->elite_count = clampi(cfg->elite_count, 1, ELITE_MAX);
    cfg->mutation_rate = clampf(cfg->mutation_rate, 0.0f, 1.0f);
    cfg->mutation_strength = clampf(cfg->mutation_strength, 0.0f, 2.0f);
    cfg->replace_fraction = clampf(cfg->replace_fraction, 0.0f, 1.0f);
}
