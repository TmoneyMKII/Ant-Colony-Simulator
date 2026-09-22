/**
 * @file spatial_hash.h
 * @brief Uniform grid over points, rebuilt every tick
 *
 * Stored in compressed-row form: cell_start[c]..cell_start[c+1] indexes
 * into items[], which holds point indices. Build is O(n + cells) with no
 * per-point allocation, and every query touches only nearby cells.
 */

#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "types.h"

typedef struct {
    float cell_size;
    float inv_cell_size;
    int cols;
    int rows;
    int *cell_start;    /**< cols*rows + 1 offsets into items */
    int *items;         /**< point indices grouped by cell */
    int *point_cell;    /**< scratch: cell of each point */
    int capacity;       /**< max points */
} SpatialHash;

bool shash_init(SpatialHash *h, int world_width, int world_height, int cell_size, int capacity);
void shash_free(SpatialHash *h);

/** @brief Rebuild from n points (points outside the grid are clamped to the edge cells) */
void shash_build(SpatialHash *h, const Vec2 *points, int n);

/**
 * @brief Collect indices of points in every cell overlapping the circle
 *
 * Candidates are not distance-filtered; callers test the exact distance.
 * @return Number of indices written (at most max_out)
 */
int shash_query(const SpatialHash *h, float x, float y, float radius, int *out, int max_out);

#endif /* SPATIAL_HASH_H */
