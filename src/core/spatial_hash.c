/**
 * @file spatial_hash.c
 * @brief Uniform grid implementation
 */

#include "spatial_hash.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

bool shash_init(SpatialHash *h, int world_width, int world_height, int cell_size, int capacity) {
    memset(h, 0, sizeof(*h));
    h->cell_size = (float)cell_size;
    h->inv_cell_size = 1.0f / (float)cell_size;
    h->cols = (world_width + cell_size - 1) / cell_size;
    h->rows = (world_height + cell_size - 1) / cell_size;
    if (h->cols < 1) h->cols = 1;
    if (h->rows < 1) h->rows = 1;
    h->capacity = capacity;

    h->cell_start = calloc((size_t)(h->cols * h->rows + 1), sizeof(int));
    h->items = malloc(sizeof(int) * (size_t)capacity);
    h->point_cell = malloc(sizeof(int) * (size_t)capacity);
    if (!h->cell_start || !h->items || !h->point_cell) {
        shash_free(h);
        return false;
    }
    return true;
}

void shash_free(SpatialHash *h) {
    free(h->cell_start);
    free(h->items);
    free(h->point_cell);
    memset(h, 0, sizeof(*h));
}

static inline int cell_coord(float v, float inv_cell, int max) {
    return clampi((int)(v * inv_cell), 0, max - 1);
}

void shash_build(SpatialHash *h, const Vec2 *points, int n) {
    const int cells = h->cols * h->rows;
    if (n > h->capacity) n = h->capacity;

    memset(h->cell_start, 0, sizeof(int) * (size_t)(cells + 1));

    /* Count points per cell (shifted by one so the prefix sum yields starts) */
    for (int i = 0; i < n; i++) {
        int cx = cell_coord(points[i].x, h->inv_cell_size, h->cols);
        int cy = cell_coord(points[i].y, h->inv_cell_size, h->rows);
        int c = cy * h->cols + cx;
        h->point_cell[i] = c;
        h->cell_start[c + 1]++;
    }

    for (int c = 0; c < cells; c++) {
        h->cell_start[c + 1] += h->cell_start[c];
    }

    /* Scatter, using point_cell as a running cursor per cell */
    for (int i = 0; i < n; i++) {
        int c = h->point_cell[i];
        h->point_cell[i] = h->cell_start[c]++;
    }
    for (int i = 0; i < n; i++) {
        h->items[h->point_cell[i]] = i;
    }

    /* The cursor pass shifted every start forward by one cell; undo it */
    for (int c = cells; c > 0; c--) {
        h->cell_start[c] = h->cell_start[c - 1];
    }
    h->cell_start[0] = 0;
}

int shash_query(const SpatialHash *h, float x, float y, float radius, int *out, int max_out) {
    int x0 = cell_coord(x - radius, h->inv_cell_size, h->cols);
    int x1 = cell_coord(x + radius, h->inv_cell_size, h->cols);
    int y0 = cell_coord(y - radius, h->inv_cell_size, h->rows);
    int y1 = cell_coord(y + radius, h->inv_cell_size, h->rows);

    int count = 0;
    for (int cy = y0; cy <= y1; cy++) {
        for (int cx = x0; cx <= x1; cx++) {
            int c = cy * h->cols + cx;
            for (int k = h->cell_start[c]; k < h->cell_start[c + 1]; k++) {
                if (count >= max_out) return count;
                out[count++] = h->items[k];
            }
        }
    }
    return count;
}
