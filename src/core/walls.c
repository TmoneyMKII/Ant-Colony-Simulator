/**
 * @file walls.c
 * @brief Wall/obstacle system with simple maze generation
 */

#include "walls.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define INITIAL_WALL_CAPACITY 256
#define SPATIAL_CELL_SIZE 64

/* ============== Spatial Hash Helpers ============== */

static void rebuild_spatial_hash(WallManager *wm) {
    int grid_size = wm->spatial_width * wm->spatial_height;
    
    /* Clear grid */
    for (int i = 0; i < grid_size; i++) {
        wm->spatial_grid[i] = -1;
    }
    
    /* Insert walls into cells */
    for (int w = 0; w < wm->wall_count; w++) {
        const Rect *wall = &wm->walls[w].bounds;
        
        /* Get cell range this wall covers */
        int cx0 = wall->x / wm->spatial_cell_size;
        int cy0 = wall->y / wm->spatial_cell_size;
        int cx1 = (wall->x + wall->width) / wm->spatial_cell_size;
        int cy1 = (wall->y + wall->height) / wm->spatial_cell_size;
        
        cx0 = clampi(cx0, 0, wm->spatial_width - 1);
        cy0 = clampi(cy0, 0, wm->spatial_height - 1);
        cx1 = clampi(cx1, 0, wm->spatial_width - 1);
        cy1 = clampi(cy1, 0, wm->spatial_height - 1);
        
        /* Add to each cell (simple linked list) */
        for (int cy = cy0; cy <= cy1; cy++) {
            for (int cx = cx0; cx <= cx1; cx++) {
                int cell_idx = cy * wm->spatial_width + cx;
                wm->wall_next[w] = wm->spatial_grid[cell_idx];
                wm->spatial_grid[cell_idx] = w;
            }
        }
    }
}

/* ============== Public Functions ============== */

WallManager* walls_create(int width, int height, int offset_x, int offset_y) {
    WallManager *wm = (WallManager*)malloc(sizeof(WallManager));
    if (!wm) return NULL;
    
    wm->width = width;
    wm->height = height;
    wm->offset_x = offset_x;
    wm->offset_y = offset_y;
    
    wm->wall_capacity = INITIAL_WALL_CAPACITY;
    wm->wall_count = 0;
    wm->walls = (WallSegment*)malloc(sizeof(WallSegment) * wm->wall_capacity);
    wm->wall_next = (int*)malloc(sizeof(int) * wm->wall_capacity);
    
    if (!wm->walls || !wm->wall_next) {
        free(wm->walls);
        free(wm->wall_next);
        free(wm);
        return NULL;
    }
    
    /* Spatial hash setup */
    wm->spatial_cell_size = SPATIAL_CELL_SIZE;
    wm->spatial_width = (width + SPATIAL_CELL_SIZE - 1) / SPATIAL_CELL_SIZE;
    wm->spatial_height = (height + SPATIAL_CELL_SIZE - 1) / SPATIAL_CELL_SIZE;
    
    int grid_size = wm->spatial_width * wm->spatial_height;
    wm->spatial_grid = (int*)malloc(sizeof(int) * grid_size);
    
    if (!wm->spatial_grid) {
        free(wm->walls);
        free(wm->wall_next);
        free(wm);
        return NULL;
    }
    
    for (int i = 0; i < grid_size; i++) {
        wm->spatial_grid[i] = -1;
    }
    
    return wm;
}

void walls_destroy(WallManager *wm) {
    if (wm) {
        free(wm->walls);
        free(wm->wall_next);
        free(wm->spatial_grid);
        free(wm);
    }
}

void walls_clear(WallManager *wm) {
    wm->wall_count = 0;
    int grid_size = wm->spatial_width * wm->spatial_height;
    for (int i = 0; i < grid_size; i++) {
        wm->spatial_grid[i] = -1;
    }
}

bool walls_add(WallManager *wm, int x, int y, int w, int h) {
    if (wm->wall_count >= wm->wall_capacity) {
        int new_cap = wm->wall_capacity * 2;
        WallSegment *new_walls = (WallSegment*)realloc(wm->walls, sizeof(WallSegment) * new_cap);
        int *new_next = (int*)realloc(wm->wall_next, sizeof(int) * new_cap);
        
        if (!new_walls || !new_next) {
            return false;
        }
        
        wm->walls = new_walls;
        wm->wall_next = new_next;
        wm->wall_capacity = new_cap;
    }
    
    wm->walls[wm->wall_count].bounds = (Rect){x, y, w, h};
    wm->wall_count++;
    
    /* Rebuild spatial hash (could be optimized to incremental add) */
    rebuild_spatial_hash(wm);
    
    return true;
}

void walls_generate_maze(WallManager *wm, float colony_x, float colony_y, float colony_radius) {
    walls_clear(wm);
    
    /* Simple maze: random rectangular obstacles */
    int num_obstacles = 20 + randi_range(0, 15);
    float colony_safe_dist = colony_radius * 3.0f;
    
    for (int i = 0; i < num_obstacles; i++) {
        int w = 40 + randi_range(0, 120);
        int h = 20 + randi_range(0, 80);
        int x = wm->offset_x + randi_range(50, wm->width - w - 50);
        int y = wm->offset_y + randi_range(50, wm->height - h - 50);
        
        /* Check not too close to colony */
        float cx = x + w / 2.0f;
        float cy = y + h / 2.0f;
        float dist = sqrtf((cx - colony_x) * (cx - colony_x) + (cy - colony_y) * (cy - colony_y));
        
        if (dist < colony_safe_dist) continue;
        
        walls_add(wm, x, y, w, h);
    }
    
    /* Add some corridor walls */
    int num_corridors = 5 + randi_range(0, 5);
    for (int i = 0; i < num_corridors; i++) {
        bool horizontal = randf() > 0.5f;
        int length = 100 + randi_range(0, 200);
        int thickness = 15 + randi_range(0, 10);
        
        int x, y, w, h;
        if (horizontal) {
            w = length;
            h = thickness;
            x = wm->offset_x + randi_range(50, wm->width - length - 50);
            y = wm->offset_y + randi_range(50, wm->height - thickness - 50);
        } else {
            w = thickness;
            h = length;
            x = wm->offset_x + randi_range(50, wm->width - thickness - 50);
            y = wm->offset_y + randi_range(50, wm->height - length - 50);
        }
        
        /* Check not too close to colony */
        float cx = x + w / 2.0f;
        float cy = y + h / 2.0f;
        float dist = sqrtf((cx - colony_x) * (cx - colony_x) + (cy - colony_y) * (cy - colony_y));
        
        if (dist < colony_safe_dist) continue;
        
        walls_add(wm, x, y, w, h);
    }
}

bool walls_is_colliding(const WallManager *wm, float x, float y, int margin, int *out_wall_idx) {
    if (!wm || wm->wall_count == 0) return false;
    
    /* Get spatial cell */
    int cx = (int)x / wm->spatial_cell_size;
    int cy = (int)y / wm->spatial_cell_size;
    
    if (cx < 0 || cx >= wm->spatial_width || cy < 0 || cy >= wm->spatial_height) {
        return false;
    }
    
    int cell_idx = cy * wm->spatial_width + cx;
    int wall_idx = wm->spatial_grid[cell_idx];
    
    int ix = (int)x;
    int iy = (int)y;
    
    while (wall_idx >= 0) {
        const Rect *wall = &wm->walls[wall_idx].bounds;
        
        if (ix >= wall->x - margin && ix < wall->x + wall->width + margin &&
            iy >= wall->y - margin && iy < wall->y + wall->height + margin) {
            if (out_wall_idx) *out_wall_idx = wall_idx;
            return true;
        }
        
        wall_idx = wm->wall_next[wall_idx];
    }
    
    return false;
}

bool walls_get_push_vector(const WallManager *wm, float x, float y, int margin,
                            float *out_dx, float *out_dy) {
    int wall_idx;
    if (!walls_is_colliding(wm, x, y, margin, &wall_idx)) {
        return false;
    }
    
    const Rect *wall = &wm->walls[wall_idx].bounds;
    
    /* Calculate push direction from wall center */
    float wall_cx = wall->x + wall->width / 2.0f;
    float wall_cy = wall->y + wall->height / 2.0f;
    
    float dx = x - wall_cx;
    float dy = y - wall_cy;
    float len = sqrtf(dx * dx + dy * dy);
    
    if (len < 0.001f) {
        *out_dx = randf() - 0.5f;
        *out_dy = randf() - 0.5f;
    } else {
        *out_dx = dx / len;
        *out_dy = dy / len;
    }
    
    return true;
}

int walls_get_count(const WallManager *wm) {
    return wm ? wm->wall_count : 0;
}

const Rect* walls_get(const WallManager *wm, int index) {
    if (!wm || index < 0 || index >= wm->wall_count) return NULL;
    return &wm->walls[index].bounds;
}
