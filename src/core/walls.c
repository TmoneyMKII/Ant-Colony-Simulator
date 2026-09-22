/**
 * @file walls.c
 * @brief Wall storage, broadphase, collision and ray casting
 */

#include "walls.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_WALL_CAPACITY 64
#define PUSH_EPSILON 0.01f

/* ============== Lifetime ============== */

bool walls_init(WallSet *ws, int world_width, int world_height) {
    memset(ws, 0, sizeof(*ws));
    ws->world_width = world_width;
    ws->world_height = world_height;
    ws->cell_size = WALL_GRID_CELL_SIZE;
    ws->cols = (world_width + ws->cell_size - 1) / ws->cell_size;
    ws->rows = (world_height + ws->cell_size - 1) / ws->cell_size;

    ws->capacity = INITIAL_WALL_CAPACITY;
    ws->rects = malloc(sizeof(Rect) * (size_t)ws->capacity);
    ws->cell_start = calloc((size_t)(ws->cols * ws->rows + 1), sizeof(int));
    if (!ws->rects || !ws->cell_start) {
        walls_free(ws);
        return false;
    }
    return true;
}

void walls_free(WallSet *ws) {
    free(ws->rects);
    free(ws->cell_start);
    free(ws->cell_items);
    memset(ws, 0, sizeof(*ws));
}

void walls_clear(WallSet *ws) {
    ws->count = 0;
    ws->dirty = true;
    walls_rebuild(ws);
}

bool walls_add(WallSet *ws, Rect r) {
    if (r.width <= 0 || r.height <= 0) return false;

    if (ws->count >= ws->capacity) {
        int new_cap = ws->capacity * 2;
        Rect *grown = realloc(ws->rects, sizeof(Rect) * (size_t)new_cap);
        if (!grown) return false;
        ws->rects = grown;
        ws->capacity = new_cap;
    }
    ws->rects[ws->count++] = r;
    ws->dirty = true;
    return true;
}

/* ============== Broadphase ============== */

/** @brief Cell range covered by an axis-aligned box (inclusive) */
static void cell_range(const WallSet *ws, float x0, float y0, float x1, float y1,
                       int *cx0, int *cy0, int *cx1, int *cy1) {
    *cx0 = clampi((int)floorf(x0 / (float)ws->cell_size), 0, ws->cols - 1);
    *cy0 = clampi((int)floorf(y0 / (float)ws->cell_size), 0, ws->rows - 1);
    *cx1 = clampi((int)floorf(x1 / (float)ws->cell_size), 0, ws->cols - 1);
    *cy1 = clampi((int)floorf(y1 / (float)ws->cell_size), 0, ws->rows - 1);
}

void walls_rebuild(WallSet *ws) {
    const int cells = ws->cols * ws->rows;
    memset(ws->cell_start, 0, sizeof(int) * (size_t)(cells + 1));

    /* Pass 1: count wall references per cell */
    int total = 0;
    for (int w = 0; w < ws->count; w++) {
        const Rect *r = &ws->rects[w];
        int cx0, cy0, cx1, cy1;
        cell_range(ws, (float)r->x, (float)r->y,
                   (float)(r->x + r->width - 1), (float)(r->y + r->height - 1),
                   &cx0, &cy0, &cx1, &cy1);
        for (int cy = cy0; cy <= cy1; cy++) {
            for (int cx = cx0; cx <= cx1; cx++) {
                ws->cell_start[cy * ws->cols + cx + 1]++;
                total++;
            }
        }
    }

    if (total > ws->cell_items_capacity) {
        int *grown = realloc(ws->cell_items, sizeof(int) * (size_t)total);
        if (!grown) return;  /* keep the stale (dirty) grid rather than crash */
        ws->cell_items = grown;
        ws->cell_items_capacity = total;
    }

    for (int c = 0; c < cells; c++) {
        ws->cell_start[c + 1] += ws->cell_start[c];
    }

    /* Pass 2: fill, using a temporary cursor per cell */
    int *cursor = malloc(sizeof(int) * (size_t)cells);
    if (!cursor) return;
    memcpy(cursor, ws->cell_start, sizeof(int) * (size_t)cells);

    for (int w = 0; w < ws->count; w++) {
        const Rect *r = &ws->rects[w];
        int cx0, cy0, cx1, cy1;
        cell_range(ws, (float)r->x, (float)r->y,
                   (float)(r->x + r->width - 1), (float)(r->y + r->height - 1),
                   &cx0, &cy0, &cx1, &cy1);
        for (int cy = cy0; cy <= cy1; cy++) {
            for (int cx = cx0; cx <= cx1; cx++) {
                ws->cell_items[cursor[cy * ws->cols + cx]++] = w;
            }
        }
    }

    free(cursor);
    ws->dirty = false;
}

/* ============== Maze generation ============== */

static bool too_close_to_nest(Rect r, Vec2 nest, float keep_clear) {
    /* Distance from nest to the nearest point of the rectangle */
    float nx = clampf(nest.x, (float)r.x, (float)(r.x + r.width));
    float ny = clampf(nest.y, (float)r.y, (float)(r.y + r.height));
    return vec2_dist_sq(nest, vec2(nx, ny)) < keep_clear * keep_clear;
}

static void try_add(WallSet *ws, Rect r, Vec2 nest, float keep_clear) {
    if (!too_close_to_nest(r, nest, keep_clear)) {
        walls_add(ws, r);
    }
}

void walls_generate_maze(WallSet *ws, Vec2 nest, float keep_clear_radius, Rng *rng) {
    ws->count = 0;
    const int margin = 50;
    const int w_max = ws->world_width;
    const int h_max = ws->world_height;

    /* Scattered block obstacles */
    int num_obstacles = 20 + rng_int(rng, 0, 15);
    for (int i = 0; i < num_obstacles; i++) {
        int w = 40 + rng_int(rng, 0, 120);
        int h = 20 + rng_int(rng, 0, 80);
        Rect r = {
            rng_int(rng, margin, w_max - w - margin),
            rng_int(rng, margin, h_max - h - margin),
            w, h
        };
        try_add(ws, r, nest, keep_clear_radius);
    }

    /* Long thin corridor walls */
    int num_corridors = 5 + rng_int(rng, 0, 5);
    for (int i = 0; i < num_corridors; i++) {
        bool horizontal = rng_float(rng) < 0.5f;
        int length = 100 + rng_int(rng, 0, 200);
        int thickness = 15 + rng_int(rng, 0, 10);
        int w = horizontal ? length : thickness;
        int h = horizontal ? thickness : length;
        Rect r = {
            rng_int(rng, margin, w_max - w - margin),
            rng_int(rng, margin, h_max - h - margin),
            w, h
        };
        try_add(ws, r, nest, keep_clear_radius);
    }

    walls_rebuild(ws);
}

/* ============== Queries ============== */

static inline Vec2 closest_point_on_rect(const Rect *r, Vec2 p) {
    return vec2(clampf(p.x, (float)r->x, (float)(r->x + r->width)),
                clampf(p.y, (float)r->y, (float)(r->y + r->height)));
}

bool walls_overlaps_circle(const WallSet *ws, float x, float y, float r) {
    int cx0, cy0, cx1, cy1;
    cell_range(ws, x - r, y - r, x + r, y + r, &cx0, &cy0, &cx1, &cy1);
    Vec2 p = vec2(x, y);

    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            int c = cy * ws->cols + cx;
            for (int k = ws->cell_start[c]; k < ws->cell_start[c + 1]; k++) {
                const Rect *wall = &ws->rects[ws->cell_items[k]];
                if (vec2_dist_sq(p, closest_point_on_rect(wall, p)) < r * r) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool walls_resolve_circle(const WallSet *ws, Vec2 *pos, float r, Vec2 *normal) {
    int cx0, cy0, cx1, cy1;
    cell_range(ws, pos->x - r, pos->y - r, pos->x + r, pos->y + r, &cx0, &cy0, &cx1, &cy1);

    Vec2 total_push = {0, 0};
    bool hit = false;

    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            int c = cy * ws->cols + cx;
            for (int k = ws->cell_start[c]; k < ws->cell_start[c + 1]; k++) {
                const Rect *wall = &ws->rects[ws->cell_items[k]];
                Vec2 closest = closest_point_on_rect(wall, *pos);
                Vec2 delta = vec2_sub(*pos, closest);
                float d_sq = vec2_len_sq(delta);
                if (d_sq >= r * r) continue;

                Vec2 push;
                if (d_sq > 1e-8f) {
                    /* Centre outside the rect: push along the contact normal */
                    float d = sqrtf(d_sq);
                    push = vec2_scale(delta, (r - d + PUSH_EPSILON) / d);
                } else {
                    /* Centre inside the rect: exit through the nearest side */
                    float left   = pos->x - (float)wall->x;
                    float right  = (float)(wall->x + wall->width) - pos->x;
                    float top    = pos->y - (float)wall->y;
                    float bottom = (float)(wall->y + wall->height) - pos->y;
                    float m = fminf(fminf(left, right), fminf(top, bottom));
                    if (m == left)       push = vec2(-(left + r + PUSH_EPSILON), 0);
                    else if (m == right) push = vec2(right + r + PUSH_EPSILON, 0);
                    else if (m == top)   push = vec2(0, -(top + r + PUSH_EPSILON));
                    else                 push = vec2(0, bottom + r + PUSH_EPSILON);
                }

                *pos = vec2_add(*pos, push);
                total_push = vec2_add(total_push, push);
                hit = true;
            }
        }
    }

    if (hit && normal) {
        float len = vec2_len(total_push);
        *normal = len > 1e-6f ? vec2_scale(total_push, 1.0f / len) : vec2(1, 0);
    }
    return hit;
}

/** @brief Slab test: entry distance of a ray into a rect, or -1 if missed */
static float ray_rect(Vec2 o, Vec2 d, const Rect *r, float max_dist) {
    float tmin = 0.0f;
    float tmax = max_dist;

    float lo[2] = {(float)r->x, (float)r->y};
    float hi[2] = {(float)(r->x + r->width), (float)(r->y + r->height)};
    float orig[2] = {o.x, o.y};
    float dir[2] = {d.x, d.y};

    for (int a = 0; a < 2; a++) {
        if (fabsf(dir[a]) < 1e-8f) {
            if (orig[a] < lo[a] || orig[a] > hi[a]) return -1.0f;
        } else {
            float inv = 1.0f / dir[a];
            float t0 = (lo[a] - orig[a]) * inv;
            float t1 = (hi[a] - orig[a]) * inv;
            if (t0 > t1) { float t = t0; t0 = t1; t1 = t; }
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax) return -1.0f;
        }
    }
    return tmin;
}

int walls_query_circle(const WallSet *ws, float x, float y, float radius, int *out, int max_out) {
    int cx0, cy0, cx1, cy1;
    cell_range(ws, x - radius, y - radius, x + radius, y + radius, &cx0, &cy0, &cx1, &cy1);

    int count = 0;
    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            int c = cy * ws->cols + cx;
            for (int k = ws->cell_start[c]; k < ws->cell_start[c + 1]; k++) {
                int wall = ws->cell_items[k];
                bool seen = false;
                for (int i = 0; i < count; i++) {
                    if (out[i] == wall) { seen = true; break; }
                }
                if (seen) continue;
                if (count >= max_out) return count;
                out[count++] = wall;
            }
        }
    }
    return count;
}

float walls_ray_rect(const WallSet *ws, int wall_index, Vec2 origin, Vec2 dir, float max_dist) {
    if (wall_index < 0 || wall_index >= ws->count) return max_dist;
    float t = ray_rect(origin, dir, &ws->rects[wall_index], max_dist);
    return t >= 0.0f ? t : max_dist;
}

float walls_raycast(const WallSet *ws, Vec2 origin, Vec2 dir, float max_dist) {
    Vec2 end = vec2_add(origin, vec2_scale(dir, max_dist));
    int cx0, cy0, cx1, cy1;
    cell_range(ws, fminf(origin.x, end.x), fminf(origin.y, end.y),
               fmaxf(origin.x, end.x), fmaxf(origin.y, end.y),
               &cx0, &cy0, &cx1, &cy1);

    float best = max_dist;
    for (int cy = cy0; cy <= cy1; cy++) {
        for (int cx = cx0; cx <= cx1; cx++) {
            int c = cy * ws->cols + cx;
            for (int k = ws->cell_start[c]; k < ws->cell_start[c + 1]; k++) {
                float t = ray_rect(origin, dir, &ws->rects[ws->cell_items[k]], best);
                if (t >= 0.0f && t < best) best = t;
            }
        }
    }
    return best;
}
