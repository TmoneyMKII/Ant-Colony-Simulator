/**
 * @file pheromone.c
 * @brief Pheromone grid implementation
 */

#include "pheromone.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

/* 8-neighbour offsets and their headings (atan2(dy, dx)) */
static const int NEIGHBOR_DX[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const int NEIGHBOR_DY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const float NEIGHBOR_ANGLE[8] = {
    -2.35619449f, -1.57079633f, -0.78539816f,
     3.14159265f,  0.0f,
     2.35619449f,  1.57079633f,  0.78539816f
};

bool pheromone_init(PheromoneMap *map, int world_width, int world_height, int cell_size) {
    memset(map, 0, sizeof(*map));
    map->cell_size = cell_size;
    map->cols = (world_width + cell_size - 1) / cell_size;
    map->rows = (world_height + cell_size - 1) / cell_size;

    size_t cells = (size_t)map->cols * (size_t)map->rows;
    for (int t = 0; t < PHEROMONE_TYPE_COUNT; t++) {
        map->layers[t] = calloc(cells, sizeof(float));
        if (!map->layers[t]) {
            pheromone_free(map);
            return false;
        }
    }
    return true;
}

void pheromone_free(PheromoneMap *map) {
    for (int t = 0; t < PHEROMONE_TYPE_COUNT; t++) {
        free(map->layers[t]);
        map->layers[t] = NULL;
    }
}

void pheromone_clear(PheromoneMap *map) {
    size_t bytes = sizeof(float) * (size_t)map->cols * (size_t)map->rows;
    for (int t = 0; t < PHEROMONE_TYPE_COUNT; t++) {
        memset(map->layers[t], 0, bytes);
    }
}

static void scale_layer(float *restrict layer, int n, float keep) {
    for (int i = 0; i < n; i++) {
        layer[i] *= keep;
    }
}

void pheromone_evaporate(PheromoneMap *map, float trail_keep, float danger_keep) {
    int n = map->cols * map->rows;
    scale_layer(map->layers[PHEROMONE_FOOD], n, trail_keep);
    scale_layer(map->layers[PHEROMONE_HOME], n, trail_keep);
    scale_layer(map->layers[PHEROMONE_DANGER], n, danger_keep);
}

static inline bool to_cell(const PheromoneMap *map, float x, float y, int *gx, int *gy) {
    if (x < 0.0f || y < 0.0f) return false;
    *gx = (int)(x / (float)map->cell_size);
    *gy = (int)(y / (float)map->cell_size);
    return *gx < map->cols && *gy < map->rows;
}

static inline float cell_value(const PheromoneMap *map, PheromoneType type, int gx, int gy) {
    if (gx < 0 || gy < 0 || gx >= map->cols || gy >= map->rows) return 0.0f;
    return map->layers[type][gy * map->cols + gx];
}

void pheromone_deposit(PheromoneMap *map, PheromoneType type, float x, float y, float amount) {
    int gx, gy;
    if (!to_cell(map, x, y, &gx, &gy)) return;
    float *cell = &map->layers[type][gy * map->cols + gx];
    *cell = fminf(PHEROMONE_MAX_VALUE, *cell + amount);
}

float pheromone_sample(const PheromoneMap *map, PheromoneType type, float x, float y) {
    int gx, gy;
    if (!to_cell(map, x, y, &gx, &gy)) return 0.0f;
    return map->layers[type][gy * map->cols + gx];
}

bool pheromone_steer(const PheromoneMap *map, PheromoneType type,
                     float x, float y, float heading, float *out_dir) {
    int gx, gy;
    if (!to_cell(map, x, y, &gx, &gy)) return false;

    float best_strength = 0.0f;
    int best = -1;

    for (int i = 0; i < 8; i++) {
        float strength = cell_value(map, type, gx + NEIGHBOR_DX[i], gy + NEIGHBOR_DY[i]);
        if (strength < PHEROMONE_DETECT_THRESHOLD) continue;

        if (angle_diff(NEIGHBOR_ANGLE[i], heading) > PI_F * 0.5f) {
            strength *= 0.3f;
        }
        if (strength > best_strength) {
            best_strength = strength;
            best = i;
        }
    }

    if (best < 0) return false;
    *out_dir = NEIGHBOR_ANGLE[best];
    return true;
}
