/**
 * @file colony.c
 * @brief Colony management implementation
 */

#include "colony.h"
#include "ant.h"
#include "neural_net.h"
#include "pheromone.h"
#include "walls.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define INITIAL_ANT_CAPACITY 256
#define INITIAL_FOOD_CAPACITY 32
#define INITIAL_MARKER_CAPACITY 128

/* ============== Colony Brain (Evolution) ============== */

static void colony_brain_init(ColonyBrain *brain) {
    memset(brain, 0, sizeof(ColonyBrain));
    brain->generation = 1;
}

static void colony_brain_cleanup(ColonyBrain *brain) {
    for (int i = 0; i < brain->elite_count; i++) {
        free(brain->elites[i].weights);
    }
}

static void colony_brain_register_ant(ColonyBrain *brain, const Ant *ant) {
    float fitness = ant_calculate_fitness(ant);
    
    if (!ant->brain) return;
    
    /* Check if this ant makes the elite list */
    int insert_idx = -1;
    for (int i = 0; i < ELITE_COUNT; i++) {
        if (i >= brain->elite_count || fitness > brain->elites[i].fitness) {
            insert_idx = i;
            break;
        }
    }
    
    if (insert_idx >= 0) {
        /* Shift others down */
        if (brain->elite_count >= ELITE_COUNT) {
            /* Free the worst elite's weights */
            free(brain->elites[ELITE_COUNT - 1].weights);
        }
        
        for (int i = (brain->elite_count < ELITE_COUNT ? brain->elite_count : ELITE_COUNT - 1); 
             i > insert_idx; i--) {
            brain->elites[i] = brain->elites[i - 1];
        }
        
        /* Insert new elite */
        int weight_count = nn_get_weight_count();
        brain->elites[insert_idx].weights = (float*)malloc(sizeof(float) * weight_count);
        nn_get_weights(ant->brain, brain->elites[insert_idx].weights);
        brain->elites[insert_idx].fitness = fitness;
        brain->elites[insert_idx].food_collected = ant->food_collected;
        
        if (brain->elite_count < ELITE_COUNT) {
            brain->elite_count++;
        }
    }
    
    brain->ants_evaluated++;
}

static NeuralNetwork* colony_brain_create_brain(ColonyBrain *brain) {
    if (brain->elite_count == 0) {
        /* No elites yet, create random brain */
        return nn_create();
    }
    
    /* Select two random elites for crossover */
    int p1_idx = randi_range(0, brain->elite_count);
    int p2_idx = randi_range(0, brain->elite_count);
    
    int weight_count = nn_get_weight_count();
    
    /* Create child from crossover */
    float *child_weights = (float*)malloc(sizeof(float) * weight_count);
    
    for (int i = 0; i < weight_count; i++) {
        if (randf() < 0.5f) {
            child_weights[i] = brain->elites[p1_idx].weights[i];
        } else {
            child_weights[i] = brain->elites[p2_idx].weights[i];
        }
    }
    
    NeuralNetwork *child = nn_create_with_weights(child_weights, weight_count);
    free(child_weights);
    
    if (child) {
        nn_mutate(child, MUTATION_RATE, MUTATION_STRENGTH);
    }
    
    return child;
}

static void colony_brain_new_generation(ColonyBrain *brain) {
    brain->generation++;
    brain->ants_evaluated = 0;
}

/* ============== Food Source Helpers ============== */

static bool is_valid_food_position(Colony *colony, float x, float y, float margin) {
    return !walls_is_colliding(colony->wall_manager, x, y, (int)margin, NULL);
}

static void create_initial_food_sources(Colony *colony) {
    float x_min = colony->bounds.x + 50.0f;
    float x_max = colony->bounds.x + colony->bounds.width - 50.0f;
    float y_min = colony->bounds.y + 50.0f;
    float y_max = colony->bounds.y + colony->bounds.height - 50.0f;
    
    for (int i = 0; i < FOOD_SOURCE_COUNT; i++) {
        for (int attempt = 0; attempt < 20; attempt++) {
            float x = randf_range(x_min, x_max);
            float y = randf_range(y_min, y_max);
            
            if (is_valid_food_position(colony, x, y, 20)) {
                float amount = randf_range(MIN_FOOD_SPAWN, MAX_FOOD_SPAWN);
                colony_add_food_source(colony, x, y, amount);
                break;
            }
        }
    }
}

/* ============== Public Functions ============== */

Colony* colony_create(float x, float y, int width, int height, Rect bounds) {
    Colony *colony = (Colony*)calloc(1, sizeof(Colony));
    if (!colony) return NULL;
    
    colony->x = x;
    colony->y = y;
    colony->width = width;
    colony->height = height;
    colony->bounds = bounds;
    colony->radius = 20.0f;
    colony->color = (Color){150, 100, 50, 255};
    
    colony->food_stored = 0;
    colony->max_food = 10000.0f;
    colony->population = 0;
    colony->max_population = MAX_POPULATION;
    
    /* Allocate ant array */
    colony->ant_capacity = INITIAL_ANT_CAPACITY;
    colony->ants = (Ant*)calloc(colony->ant_capacity, sizeof(Ant));
    if (!colony->ants) {
        free(colony);
        return NULL;
    }
    
    /* Allocate food sources */
    colony->food_capacity = INITIAL_FOOD_CAPACITY;
    colony->food_sources = (FoodSource*)calloc(colony->food_capacity, sizeof(FoodSource));
    if (!colony->food_sources) {
        free(colony->ants);
        free(colony);
        return NULL;
    }
    
    /* Allocate death markers */
    colony->death_marker_capacity = INITIAL_MARKER_CAPACITY;
    colony->death_markers = (DeathMarker*)calloc(colony->death_marker_capacity, sizeof(DeathMarker));
    if (!colony->death_markers) {
        free(colony->food_sources);
        free(colony->ants);
        free(colony);
        return NULL;
    }
    
    /* Create subsystems */
    colony->pheromone_map = pheromone_create(width, height, PHEROMONE_CELL_SIZE);
    colony->wall_manager = walls_create(width, height, bounds.x, bounds.y);
    
    if (!colony->pheromone_map || !colony->wall_manager) {
        colony_destroy(colony);
        return NULL;
    }
    
    /* Generate maze */
    walls_generate_maze(colony->wall_manager, x, y, colony->radius);
    
    /* Colony brain for evolution */
    colony->brain = (ColonyBrain*)malloc(sizeof(ColonyBrain));
    if (!colony->brain) {
        colony_destroy(colony);
        return NULL;
    }
    colony_brain_init(colony->brain);
    
    /* Create initial food sources */
    create_initial_food_sources(colony);
    
    /* Spawn initial ants */
    for (int i = 0; i < INITIAL_ANT_COUNT; i++) {
        colony_spawn_ant(colony);
    }
    
    return colony;
}

void colony_destroy(Colony *colony) {
    if (!colony) return;
    
    /* Cleanup ants */
    for (int i = 0; i < colony->ant_count; i++) {
        ant_cleanup(&colony->ants[i]);
    }
    free(colony->ants);
    
    free(colony->food_sources);
    free(colony->death_markers);
    
    if (colony->brain) {
        colony_brain_cleanup(colony->brain);
        free(colony->brain);
    }
    
    pheromone_destroy(colony->pheromone_map);
    walls_destroy(colony->wall_manager);
    
    free(colony);
}

void colony_update(Colony *colony) {
    colony->time++;
    colony->food_collected_this_frame = 0;
    
    /* Update pheromones */
    pheromone_update(colony->pheromone_map);
    
    /* Update ants */
    for (int i = 0; i < colony->ant_count; i++) {
        Ant *ant = &colony->ants[i];
        
        if (!ant->alive) continue;
        
        bool survived = ant_update(ant,
                                    colony->pheromone_map,
                                    colony->food_sources, colony->food_count,
                                    colony->ants, colony->ant_count,
                                    colony->wall_manager,
                                    &colony->bounds);
        
        if (!survived) {
            /* Ant died - add death marker and pheromone */
            colony_add_death_marker(colony, ant->x, ant->y);
            pheromone_deposit_danger(colony->pheromone_map, ant->x, ant->y, 150);
            
            /* Register for evolution */
            colony_brain_register_ant(colony->brain, ant);
            
            /* Respawn at colony */
            float angle = randf_range(0, (float)(2 * M_PI));
            float dist = randf_range(0, colony->radius + 10);
            float new_x = colony->x + cosf(angle) * dist;
            float new_y = colony->y + sinf(angle) * dist;
            
            ant_cleanup(ant);
            ant_init(ant, new_x, new_y, ant->id, colony);
            ant->brain = colony_brain_create_brain(colony->brain);
        }
    }
    
    /* Update death markers */
    int marker_write = 0;
    for (int i = 0; i < colony->death_marker_count; i++) {
        colony->death_markers[i].frames_remaining--;
        if (colony->death_markers[i].frames_remaining > 0) {
            if (marker_write != i) {
                colony->death_markers[marker_write] = colony->death_markers[i];
            }
            marker_write++;
        }
    }
    colony->death_marker_count = marker_write;
    
    /* Generation evolution timer */
    colony->generation_timer++;
    if (colony->generation_timer >= GENERATION_INTERVAL) {
        colony_brain_new_generation(colony->brain);
        colony->generation_timer = 0;
    }
    
    /* Respawn depleted food sources occasionally */
    if (colony->time % 600 == 0) {  /* Every 10 seconds */
        for (int i = 0; i < colony->food_count; i++) {
            if (colony->food_sources[i].amount <= 0) {
                /* Regenerate at new random position */
                float x_min = colony->bounds.x + 50.0f;
                float x_max = colony->bounds.x + colony->bounds.width - 50.0f;
                float y_min = colony->bounds.y + 50.0f;
                float y_max = colony->bounds.y + colony->bounds.height - 50.0f;
                
                for (int attempt = 0; attempt < 10; attempt++) {
                    float x = randf_range(x_min, x_max);
                    float y = randf_range(y_min, y_max);
                    
                    if (is_valid_food_position(colony, x, y, 20)) {
                        colony->food_sources[i].x = x;
                        colony->food_sources[i].y = y;
                        colony->food_sources[i].amount = randf_range(MIN_FOOD_SPAWN, MAX_FOOD_SPAWN);
                        colony->food_sources[i].max_amount = colony->food_sources[i].amount;
                        break;
                    }
                }
            }
        }
    }
}

bool colony_spawn_ant(Colony *colony) {
    if (colony->ant_count >= colony->max_population) {
        return false;
    }
    
    /* Expand array if needed */
    if (colony->ant_count >= colony->ant_capacity) {
        int new_cap = colony->ant_capacity * 2;
        Ant *new_ants = (Ant*)realloc(colony->ants, sizeof(Ant) * new_cap);
        if (!new_ants) return false;
        colony->ants = new_ants;
        colony->ant_capacity = new_cap;
    }
    
    /* Spawn near colony */
    float angle = randf_range(0, (float)(2 * M_PI));
    float dist = randf_range(0, colony->radius + 10);
    float x = colony->x + cosf(angle) * dist;
    float y = colony->y + sinf(angle) * dist;
    
    Ant *ant = &colony->ants[colony->ant_count];
    ant_init(ant, x, y, (uint32_t)colony->ant_count, colony);
    ant->brain = colony_brain_create_brain(colony->brain);
    
    colony->ant_count++;
    colony->population++;
    
    return true;
}

void colony_add_food(Colony *colony, float amount) {
    colony->food_stored = fminf(colony->food_stored + amount, colony->max_food);
}

bool colony_add_food_source(Colony *colony, float x, float y, float amount) {
    if (!is_valid_food_position(colony, x, y, 10)) {
        return false;
    }
    
    /* Expand array if needed */
    if (colony->food_count >= colony->food_capacity) {
        int new_cap = colony->food_capacity * 2;
        FoodSource *new_food = (FoodSource*)realloc(colony->food_sources, sizeof(FoodSource) * new_cap);
        if (!new_food) return false;
        colony->food_sources = new_food;
        colony->food_capacity = new_cap;
    }
    
    FoodSource *food = &colony->food_sources[colony->food_count];
    food->x = x;
    food->y = y;
    food->amount = amount;
    food->max_amount = amount;
    food->radius = 10.0f;
    food->color = (Color){200, 150, 50, 255};
    
    colony->food_count++;
    return true;
}

void colony_add_death_marker(Colony *colony, float x, float y) {
    if (colony->death_marker_count >= MAX_DEATH_MARKERS) {
        /* Remove oldest marker */
        memmove(&colony->death_markers[0], &colony->death_markers[1], 
                sizeof(DeathMarker) * (colony->death_marker_count - 1));
        colony->death_marker_count--;
    }
    
    /* Expand if needed (shouldn't happen with MAX_DEATH_MARKERS check) */
    if (colony->death_marker_count >= colony->death_marker_capacity) {
        int new_cap = colony->death_marker_capacity * 2;
        DeathMarker *new_markers = (DeathMarker*)realloc(colony->death_markers, sizeof(DeathMarker) * new_cap);
        if (!new_markers) return;
        colony->death_markers = new_markers;
        colony->death_marker_capacity = new_cap;
    }
    
    DeathMarker *marker = &colony->death_markers[colony->death_marker_count];
    marker->x = x;
    marker->y = y;
    marker->frames_remaining = DEATH_MARKER_DURATION;
    
    colony->death_marker_count++;
}

void colony_reset(Colony *colony) {
    /* Cleanup existing ants */
    for (int i = 0; i < colony->ant_count; i++) {
        ant_cleanup(&colony->ants[i]);
    }
    colony->ant_count = 0;
    colony->population = 0;
    
    /* Clear food sources */
    colony->food_count = 0;
    
    /* Clear death markers */
    colony->death_marker_count = 0;
    
    /* Clear pheromones */
    pheromone_clear(colony->pheromone_map);
    
    /* Regenerate maze */
    walls_generate_maze(colony->wall_manager, colony->x, colony->y, colony->radius);
    
    /* Reset brain */
    colony_brain_cleanup(colony->brain);
    colony_brain_init(colony->brain);
    
    /* Reset colony state */
    colony->food_stored = 0;
    colony->time = 0;
    colony->generation_timer = 0;
    
    /* Create fresh food and ants */
    create_initial_food_sources(colony);
    for (int i = 0; i < INITIAL_ANT_COUNT; i++) {
        colony_spawn_ant(colony);
    }
}

void colony_get_stats(const Colony *colony,
                      int *out_population,
                      float *out_food_stored,
                      int *out_generation,
                      float *out_best_fitness) {
    if (out_population) *out_population = colony->population;
    if (out_food_stored) *out_food_stored = colony->food_stored;
    if (out_generation) *out_generation = colony->brain->generation;
    if (out_best_fitness) {
        *out_best_fitness = colony->brain->elite_count > 0 ? 
                            colony->brain->elites[0].fitness : 0.0f;
    }
}
