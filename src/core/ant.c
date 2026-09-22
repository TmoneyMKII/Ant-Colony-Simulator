/**
 * @file ant.c
 * @brief Ant agent behavior implementation
 * 
 * State machine:
 *   FORAGING  - Looking for food, following food pheromone trails
 *   RETURNING - Carrying food, following home pheromone trails
 *   IDLE      - Resting at colony
 */

#include "ant.h"
#include "neural_net.h"
#include "pheromone.h"
#include "vision.h"
#include "walls.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============== Initialization ============== */

void ant_init(Ant *ant, float x, float y, uint32_t id, Colony *colony) {
    memset(ant, 0, sizeof(Ant));
    
    ant->id = id;
    ant->x = x;
    ant->y = y;
    ant->colony = colony;
    
    ant->direction = randf_range(0, (float)(2 * M_PI));
    ant->prev_direction = ant->direction;
    ant->speed = DEFAULT_ANT_SPEED;
    
    ant->state = ANT_STATE_FORAGING;
    ant->alive = true;
    ant->carrying_food = false;
    ant->food_amount = 0;
    ant->energy = 100.0f;
    ant->max_energy = 100.0f;
    
    ant->pheromone_strength = DEFAULT_PHEROMONE_STRENGTH;
    ant->pheromone_sensitivity = DEFAULT_PHEROMONE_SENSITIVITY;
    ant->can_deposit = true;
    ant->deposit_cooldown = 0;
    
    ant->checkpoint_x = x;
    ant->checkpoint_y = y;
    ant->movement_timer = 0;
    ant->stuck_escape_count = 0;
    
    ant->use_neural_net = true;
    ant->brain = NULL;  /* Set by colony */
    
    ant->radius = 6.0f;
    ant->color = (Color){220, 120, 180, 255};
    
    vision_init(&ant->vision);
}

void ant_cleanup(Ant *ant) {
    if (ant->brain) {
        nn_destroy(ant->brain);
        ant->brain = NULL;
    }
}

/* ============== Helper Functions ============== */

static inline float dist_to_colony(const Ant *ant) {
    return dist(ant->x, ant->y, ant->colony->x, ant->colony->y);
}

static inline float dir_to_colony(const Ant *ant) {
    return atan2f(ant->colony->y - ant->y, ant->colony->x - ant->x);
}

static void constrain_to_bounds(Ant *ant, const Rect *bounds) {
    float margin = ant->radius + 5;
    
    if (ant->x < bounds->x + margin) {
        ant->x = bounds->x + margin;
        ant->direction = randf_range(-M_PI/2, M_PI/2);
    }
    if (ant->x > bounds->x + bounds->width - margin) {
        ant->x = bounds->x + bounds->width - margin;
        ant->direction = randf_range(M_PI/2, 3*M_PI/2);
    }
    if (ant->y < bounds->y + margin) {
        ant->y = bounds->y + margin;
        ant->direction = randf_range(0, M_PI);
    }
    if (ant->y > bounds->y + bounds->height - margin) {
        ant->y = bounds->y + bounds->height - margin;
        ant->direction = randf_range(-M_PI, 0);
    }
}

static void handle_wall_collision(Ant *ant, const WallManager *wm) {
    if (!wm) return;
    
    float push_x, push_y;
    if (walls_get_push_vector(wm, ant->x, ant->y, (int)ant->radius, &push_x, &push_y)) {
        /* Push out of wall */
        ant->x += push_x * (ant->radius + 2);
        ant->y += push_y * (ant->radius + 2);
        
        /* Turn away from wall */
        ant->direction = atan2f(push_y, push_x) + randf_range(-0.5f, 0.5f);
    }
}

/* ============== Neural Network Integration ============== */

void ant_get_nn_inputs(Ant *ant, float *inputs, const PheromoneMap *pheromone_map) {
    /* Vision inputs [0-20]: wall×7, ant×7, food×7 */
    vision_get_inputs(&ant->vision, inputs);
    
    /* State inputs [21-26] */
    int idx = NN_VISION_INPUTS;
    
    /* Food pheromone strength ahead */
    float ahead_x = ant->x + cosf(ant->direction) * 20;
    float ahead_y = ant->y + sinf(ant->direction) * 20;
    inputs[idx++] = clampf(pheromone_get_strength(pheromone_map, ahead_x, ahead_y, 
                           PHEROMONE_FOOD_TRAIL) / PHEROMONE_MAX_VALUE, 0, 1);
    
    /* Home pheromone strength ahead */
    inputs[idx++] = clampf(pheromone_get_strength(pheromone_map, ahead_x, ahead_y,
                           PHEROMONE_HOME_TRAIL) / PHEROMONE_MAX_VALUE, 0, 1);
    
    /* Distance to colony (normalized) */
    float colony_dist = dist_to_colony(ant);
    float max_dist = sqrtf((float)(ant->colony->width * ant->colony->width + 
                                   ant->colony->height * ant->colony->height));
    inputs[idx++] = clampf(colony_dist / max_dist, 0, 1);
    
    /* Direction to colony (normalized -1 to 1) */
    float colony_dir = dir_to_colony(ant);
    float angle_diff_val = normalize_angle(colony_dir - ant->direction);
    inputs[idx++] = angle_diff_val / (float)M_PI;
    
    /* Carrying food (0 or 1) */
    inputs[idx++] = ant->carrying_food ? 1.0f : 0.0f;
    
    /* Energy level (0 to 1) */
    inputs[idx++] = ant->energy / ant->max_energy;
}

void ant_apply_nn_outputs(Ant *ant, const float *outputs) {
    /* Output 0: Turn amount (-1 to 1) -> radians */
    float turn = outputs[0] * ANT_WANDER_TURN_RATE * 2.0f;
    ant->direction = normalize_angle(ant->direction + turn);
    
    /* Output 1: Speed modifier (mapped to 0.5 - 1.5) */
    float speed_mod = 0.5f + (outputs[1] + 1.0f) * 0.5f;
    ant->speed = DEFAULT_ANT_SPEED * clampf(speed_mod, 0.5f, 1.5f);
    
    /* Output 2: Exploration (affects randomness) - used for probabilistic behavior */
    /* Higher exploration = more random turning */
    if (outputs[2] > 0.5f) {
        ant->direction += randf_range(-0.2f, 0.2f);
    }
}

/* ============== Behavior Functions ============== */

static void deposit_pheromone(Ant *ant, PheromoneMap *map) {
    if (!ant->can_deposit || ant->deposit_cooldown > 0) {
        ant->deposit_cooldown--;
        return;
    }
    
    float amount = PHEROMONE_DEPOSIT * ant->pheromone_strength;
    
    if (ant->state == ANT_STATE_RETURNING && ant->carrying_food) {
        /* Returning with food: deposit FOOD_TRAIL to lead others to food */
        pheromone_deposit_food(map, ant->x, ant->y, amount);
    } else if (ant->state == ANT_STATE_FORAGING) {
        /* Foraging: deposit HOME_TRAIL to help return home */
        pheromone_deposit_home(map, ant->x, ant->y, amount);
    }
    
    ant->deposit_cooldown = 3;  /* Deposit every few frames */
}

static void check_food_pickup(Ant *ant, FoodSource *food_sources, int food_count) {
    if (ant->carrying_food) return;
    
    for (int i = 0; i < food_count; i++) {
        FoodSource *food = &food_sources[i];
        if (food->amount <= 0) continue;
        
        float d_sq = dist_sq(ant->x, ant->y, food->x, food->y);
        if (d_sq < ANT_FOOD_PICKUP_RANGE_SQ) {
            /* Pick up food */
            float pickup = fminf(1.0f, food->amount);
            food->amount -= pickup;
            ant->food_amount = pickup;
            ant->carrying_food = true;
            
            /* Remember food location */
            ant->knows_food_location = true;
            ant->last_food_x = food->x;
            ant->last_food_y = food->y;
            
            /* Switch to returning state */
            ant->state = ANT_STATE_RETURNING;
            
            /* Turn toward colony */
            ant->direction = dir_to_colony(ant);
            break;
        }
    }
}

static bool check_food_dropoff(Ant *ant) {
    if (!ant->carrying_food) return false;
    
    float d_sq = dist_sq(ant->x, ant->y, ant->colony->x, ant->colony->y);
    if (d_sq < ANT_COLONY_DROPOFF_RANGE_SQ) {
        /* Drop off food */
        ant->colony->food_stored += ant->food_amount;
        ant->colony->food_collected_this_frame++;
        ant->food_amount = 0;
        ant->carrying_food = false;
        
        /* Stats */
        ant->food_collected++;
        ant->successful_trips++;
        
        /* Switch back to foraging */
        ant->state = ANT_STATE_FORAGING;
        
        /* If we remember food location, head back there */
        if (ant->knows_food_location) {
            ant->direction = atan2f(ant->last_food_y - ant->y, 
                                    ant->last_food_x - ant->x);
        } else {
            ant->direction = randf_range(0, (float)(2 * M_PI));
        }
        
        /* Energy boost for successful trip */
        ant->energy = fminf(ant->max_energy, ant->energy + 20.0f);
        
        return true;
    }
    return false;
}

static void do_movement(Ant *ant) {
    ant->prev_direction = ant->direction;
    
    float dx = cosf(ant->direction) * ant->speed;
    float dy = sinf(ant->direction) * ant->speed;
    
    ant->x += dx;
    ant->y += dy;
}

static bool check_stuck(Ant *ant) {
    ant->movement_timer++;
    
    if (ant->movement_timer >= STUCK_CHECK_INTERVAL) {
        float d_sq = dist_sq(ant->x, ant->y, ant->checkpoint_x, ant->checkpoint_y);
        
        if (d_sq < STUCK_MIN_MOVEMENT_SQ) {
            ant->stuck_escape_count++;
            
            if (ant->stuck_escape_count >= MAX_ESCAPE_ATTEMPTS) {
                /* Ant dies if stuck too long */
                return true;  /* Died */
            }
            
            /* Try to escape: random direction */
            ant->direction = randf_range(0, (float)(2 * M_PI));
        } else {
            ant->stuck_escape_count = 0;
        }
        
        ant->checkpoint_x = ant->x;
        ant->checkpoint_y = ant->y;
        ant->movement_timer = 0;
    }
    
    return false;
}

/* ============== Main Update ============== */

bool ant_update(Ant *ant,
                PheromoneMap *pheromone_map,
                FoodSource *food_sources, int food_count,
                Ant *all_ants, int ant_count,
                const WallManager *wall_manager,
                const Rect *bounds) {
    
    if (!ant->alive) return false;
    
    ant->time_alive++;
    
    /* Energy consumption */
    ant->energy -= DEFAULT_ENERGY_EFFICIENCY;
    if (ant->energy <= 0) {
        ant->alive = false;
        return false;
    }
    
    /* Cast vision rays */
    vision_cast_rays(&ant->vision,
                     ant->x, ant->y, ant->direction,
                     wall_manager,
                     all_ants, ant_count,
                     food_sources, food_count,
                     ant->id);
    
    /* Neural network decision making */
    if (ant->use_neural_net && ant->brain) {
        float inputs[NN_INPUT_SIZE];
        float outputs[NN_OUTPUT_SIZE];
        
        ant_get_nn_inputs(ant, inputs, pheromone_map);
        nn_forward(ant->brain, inputs, outputs);
        ant_apply_nn_outputs(ant, outputs);
    } else {
        /* Classic behavior: follow pheromones */
        float pheromone_dir;
        bool found_trail = false;
        
        if (ant->state == ANT_STATE_FORAGING) {
            found_trail = pheromone_get_direction(pheromone_map, ant->x, ant->y,
                                                   PHEROMONE_FOOD_TRAIL, ant->direction,
                                                   &pheromone_dir);
        } else if (ant->state == ANT_STATE_RETURNING) {
            found_trail = pheromone_get_direction(pheromone_map, ant->x, ant->y,
                                                   PHEROMONE_HOME_TRAIL, ant->direction,
                                                   &pheromone_dir);
        }
        
        if (found_trail) {
            /* Blend toward pheromone direction */
            float blend = ant->pheromone_sensitivity;
            ant->direction = lerpf(ant->direction, pheromone_dir, blend);
        } else {
            /* Random wandering */
            ant->direction += randf_range(-ANT_WANDER_TURN_RATE, ANT_WANDER_TURN_RATE);
        }
    }
    
    /* Movement */
    do_movement(ant);
    
    /* Wall collision */
    handle_wall_collision(ant, wall_manager);
    
    /* Bounds constraint */
    if (bounds) {
        constrain_to_bounds(ant, bounds);
    }
    
    /* Stuck detection */
    if (check_stuck(ant)) {
        ant->alive = false;
        return false;
    }
    
    /* State-specific behavior */
    if (ant->state == ANT_STATE_FORAGING) {
        check_food_pickup(ant, food_sources, food_count);
    } else if (ant->state == ANT_STATE_RETURNING) {
        check_food_dropoff(ant);
    }
    
    /* Deposit pheromones */
    deposit_pheromone(ant, pheromone_map);
    
    return true;
}

void ant_set_state(Ant *ant, AntState state) {
    ant->state = state;
}

float ant_calculate_fitness(const Ant *ant) {
    /* Fitness based on food collected and survival time */
    float food_score = (float)ant->food_collected * 10.0f;
    float trip_score = (float)ant->successful_trips * 5.0f;
    float time_score = (float)ant->time_alive * 0.01f;
    
    return food_score + trip_score + time_score;
}
