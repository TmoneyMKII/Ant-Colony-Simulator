/**
 * @file walls.h
 * @brief Rectangular obstacles with a uniform-grid broadphase
 */

#ifndef WALLS_H
#define WALLS_H

#include "types.h"
#include "utils.h"

typedef struct {
    Rect *rects;
    int count;
    int capacity;

    int world_width;
    int world_height;

    /* Broadphase: compressed-row grid of wall indices (a wall appears in
       every cell it overlaps). Rebuilt by walls_rebuild(). */
    int cell_size;
    int cols;
    int rows;
    int *cell_start;    /**< cols*rows + 1 offsets into cell_items */
    int *cell_items;
    int cell_items_capacity;
    bool dirty;         /**< walls changed since the last rebuild */
} WallSet;

bool walls_init(WallSet *ws, int world_width, int world_height);
void walls_free(WallSet *ws);
void walls_clear(WallSet *ws);

/** @brief Add a wall; call walls_rebuild() before querying */
bool walls_add(WallSet *ws, Rect r);

/** @brief Rebuild the broadphase after adding walls */
void walls_rebuild(WallSet *ws);

/** @brief Random obstacles and corridors, keeping clear of the nest */
void walls_generate_maze(WallSet *ws, Vec2 nest, float keep_clear_radius, Rng *rng);

/** @brief True if a circle of radius r at (x, y) overlaps any wall */
bool walls_overlaps_circle(const WallSet *ws, float x, float y, float r);

/**
 * @brief Push a circle out of any walls it overlaps
 * @param normal Receives the combined push direction (unit length) if resolved
 * @return true if the circle was overlapping and has been moved
 */
bool walls_resolve_circle(const WallSet *ws, Vec2 *pos, float r, Vec2 *normal);

/** @brief Distance along a unit ray to the first wall, or max_dist if none */
float walls_raycast(const WallSet *ws, Vec2 origin, Vec2 dir, float max_dist);

/**
 * @brief Indices of walls near a circle, without duplicates
 *
 * Lets a caller that casts several rays from one point walk the grid once.
 * @return Number of indices written (at most max_out)
 */
int walls_query_circle(const WallSet *ws, float x, float y, float radius, int *out, int max_out);

/** @brief Distance along a unit ray to one wall, or max_dist if it misses */
float walls_ray_rect(const WallSet *ws, int wall_index, Vec2 origin, Vec2 dir, float max_dist);

#endif /* WALLS_H */
