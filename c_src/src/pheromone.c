/**
 * @file pheromone.c
 * @brief Pheromone trail system implementation
 * 
 * Three pheromone types:
 *   FOOD_TRAIL (Green): Deposited by returning ants, leads TO food
 *   HOME_TRAIL (Blue):  Deposited by foraging ants, leads TO home
 *   DANGER (Red):       Deposited where ants die, deters others
 */

#include "pheromone.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============== Helper Functions ============== */

static PheromoneLayer* layer_create(int grid_width, int grid_height) {
    PheromoneLayer *layer = (PheromoneLayer*)malloc(sizeof(PheromoneLayer));
    if (!layer) return NULL;
    
    layer->grid_width = grid_width;
    layer->grid_height = grid_height;
    
    int size = grid_width * grid_height;
    layer->grid = (float*)calloc(size, sizeof(float));
    if (!layer->grid) {
        free(layer);
        return NULL;
    }
    
    return layer;
}

static void layer_destroy(PheromoneLayer *layer) {
    if (layer) {
        free(layer->grid);
    }
}

static inline float* layer_get_cell(PheromoneLayer *layer, int gx, int gy) {
    if (gx < 0 || gx >= layer->grid_width || gy < 0 || gy >= layer->grid_height) {
        return NULL;
    }
    return &layer->grid[gy * layer->grid_width + gx];
}

static void layer_deposit(PheromoneLayer *layer, int gx, int gy, float amount, float max_value) {
    float *cell = layer_get_cell(layer, gx, gy);
    if (cell) {
        *cell = fminf(max_value, *cell + amount);
    }
}

static float layer_get(const PheromoneLayer *layer, int gx, int gy) {
    if (gx < 0 || gx >= layer->grid_width || gy < 0 || gy >= layer->grid_height) {
        return 0.0f;
    }
    return layer->grid[gy * layer->grid_width + gx];
}

static void layer_evaporate(PheromoneLayer *layer, float rate) {
    int size = layer->grid_width * layer->grid_height;
    float *grid = layer->grid;
    
    /* Vectorizable loop */
    for (int i = 0; i < size; i++) {
        grid[i] *= rate;
    }
}

static void layer_clear(PheromoneLayer *layer) {
    memset(layer->grid, 0, sizeof(float) * layer->grid_width * layer->grid_height);
}

/* ============== PheromoneMap Implementation ============== */

PheromoneMap* pheromone_create(int width, int height, int cell_size) {
    PheromoneMap *map = (PheromoneMap*)malloc(sizeof(PheromoneMap));
    if (!map) return NULL;
    
    map->width = width;
    map->height = height;
    map->cell_size = cell_size;
    map->grid_width = width / cell_size;
    map->grid_height = height / cell_size;
    
    /* Create layers */
    PheromoneLayer *food = layer_create(map->grid_width, map->grid_height);
    PheromoneLayer *home = layer_create(map->grid_width, map->grid_height);
    PheromoneLayer *danger = layer_create(map->grid_width, map->grid_height);
    
    if (!food || !home || !danger) {
        layer_destroy(food);
        layer_destroy(home);
        layer_destroy(danger);
        free(map);
        return NULL;
    }
    
    map->food_trail = *food;
    map->home_trail = *home;
    map->danger_trail = *danger;
    
    /* Don't need the container structs anymore (data is copied) */
    free(food);
    free(home);
    free(danger);
    
    /* Configuration */
    map->max_pheromone = PHEROMONE_MAX_VALUE;
    map->evaporation_rate = PHEROMONE_EVAP_RATE;
    map->danger_evap_rate = PHEROMONE_DANGER_EVAP;
    map->detection_threshold = PHEROMONE_DETECT_THRESHOLD;
    
    return map;
}

void pheromone_destroy(PheromoneMap *map) {
    if (map) {
        free(map->food_trail.grid);
        free(map->home_trail.grid);
        free(map->danger_trail.grid);
        free(map);
    }
}

void pheromone_clear(PheromoneMap *map) {
    layer_clear(&map->food_trail);
    layer_clear(&map->home_trail);
    layer_clear(&map->danger_trail);
}

void pheromone_update(PheromoneMap *map) {
    layer_evaporate(&map->food_trail, map->evaporation_rate);
    layer_evaporate(&map->home_trail, map->evaporation_rate);
    layer_evaporate(&map->danger_trail, map->danger_evap_rate);
}

static inline PheromoneLayer* get_layer(PheromoneMap *map, PheromoneType type) {
    switch (type) {
        case PHEROMONE_FOOD_TRAIL: return &map->food_trail;
        case PHEROMONE_DANGER:     return &map->danger_trail;
        default:                   return &map->home_trail;
    }
}

static inline const PheromoneLayer* get_layer_const(const PheromoneMap *map, PheromoneType type) {
    switch (type) {
        case PHEROMONE_FOOD_TRAIL: return &map->food_trail;
        case PHEROMONE_DANGER:     return &map->danger_trail;
        default:                   return &map->home_trail;
    }
}

void pheromone_to_grid(const PheromoneMap *map, float x, float y, int *gx, int *gy) {
    *gx = clampi((int)(x / map->cell_size), 0, map->grid_width - 1);
    *gy = clampi((int)(y / map->cell_size), 0, map->grid_height - 1);
}

void pheromone_deposit(PheromoneMap *map, float x, float y, float amount, PheromoneType type) {
    int gx, gy;
    pheromone_to_grid(map, x, y, &gx, &gy);
    layer_deposit(get_layer(map, type), gx, gy, amount, map->max_pheromone);
}

float pheromone_get_strength(const PheromoneMap *map, float x, float y, PheromoneType type) {
    int gx, gy;
    pheromone_to_grid(map, x, y, &gx, &gy);
    return layer_get(get_layer_const(map, type), gx, gy);
}

bool pheromone_get_direction(const PheromoneMap *map, float x, float y, 
                              PheromoneType type, float current_dir, float *out_dir) {
    int gx, gy;
    pheromone_to_grid(map, x, y, &gx, &gy);
    
    const PheromoneLayer *layer = get_layer_const(map, type);
    
    /* 8-directional neighbor offsets */
    static const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    float best_strength = 0.0f;
    float best_dir = 0.0f;
    bool found = false;
    
    bool use_forward_bias = !isnan(current_dir);
    
    for (int i = 0; i < 8; i++) {
        int nx = gx + dx[i];
        int ny = gy + dy[i];
        
        float strength = layer_get(layer, nx, ny);
        
        if (strength < map->detection_threshold) {
            continue;
        }
        
        /* Apply forward bias if current direction provided */
        if (use_forward_bias) {
            float target_dir = atan2f((float)dy[i], (float)dx[i]);
            float angle_diff_val = angle_diff(target_dir, current_dir);
            
            /* Penalize backwards directions (>90 degrees) */
            if (angle_diff_val > M_PI / 2) {
                strength *= 0.3f;
            }
        }
        
        if (strength > best_strength) {
            best_strength = strength;
            best_dir = atan2f((float)dy[i], (float)dx[i]);
            found = true;
        }
    }
    
    if (found) {
        *out_dir = best_dir;
    }
    return found;
}

float pheromone_get_danger(const PheromoneMap *map, float x, float y) {
    return pheromone_get_strength(map, x, y, PHEROMONE_DANGER);
}

void pheromone_get_grid_size(const PheromoneMap *map, int *width, int *height) {
    *width = map->grid_width;
    *height = map->grid_height;
}

float pheromone_get_cell(const PheromoneMap *map, int gx, int gy, PheromoneType type) {
    return layer_get(get_layer_const(map, type), gx, gy);
}
