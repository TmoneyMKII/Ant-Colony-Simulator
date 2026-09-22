/**
 * @file vision.c
 * @brief Ray casting for ant senses
 *
 * Nearby ants and food are gathered once per ant (through the world's
 * spatial grid), then every ray is tested against that small candidate
 * list instead of against the whole colony.
 */

#include "vision.h"
#include "world.h"
#include "utils.h"
#include <string.h>

#define MAX_ANT_CANDIDATES  512
#define MAX_WALL_CANDIDATES 64

static float ray_angles[NN_NUM_VISION_RAYS];
static bool ray_angles_ready = false;

static void init_ray_angles(void) {
    float half_fov_rad = (VISION_FOV_DEGREES * 0.5f) * PI_F / 180.0f;
    for (int i = 0; i < NN_NUM_VISION_RAYS; i++) {
        float t = (NN_NUM_VISION_RAYS > 1) ? (float)i / (float)(NN_NUM_VISION_RAYS - 1) : 0.5f;
        ray_angles[i] = -half_fov_rad + t * 2.0f * half_fov_rad;
    }
    ray_angles_ready = true;
}

float vision_ray_angle(int i) {
    if (!ray_angles_ready) init_ray_angles();
    return (i >= 0 && i < NN_NUM_VISION_RAYS) ? ray_angles[i] : 0.0f;
}

/**
 * @brief Test one circle against every ray, sharing the per-circle work
 *
 * All rays start at the same point, so the vector to the circle and its
 * squared length are computed once; each ray then costs a dot product.
 */
static void test_circle_against_rays(Vec2 delta, float delta_len_sq, float radius,
                                     const Vec2 *ray_dirs, float *dists) {
    const float radius_sq = radius * radius;

    for (int r = 0; r < NN_NUM_VISION_RAYS; r++) {
        float along = vec2_dot(delta, ray_dirs[r]);
        if (along < 0.0f) continue;                 /* behind this ray */
        if (along - radius >= dists[r]) continue;   /* farther than the current hit */

        float perp_sq = delta_len_sq - along * along;
        if (perp_sq > radius_sq) continue;          /* ray passes beside it */

        float hit = along - sqrtf(radius_sq - perp_sq);
        if (hit < 0.0f) hit = 0.0f;                 /* ray starts inside the circle */
        if (hit < dists[r]) dists[r] = hit;
    }
}

void vision_cast(const World *w, int self_index, Vec2 pos, float heading,
                 VisionRay out[NN_NUM_VISION_RAYS]) {
    if (!ray_angles_ready) init_ray_angles();

    const float range = VISION_RAY_LENGTH;

    /* Ray directions are shared by every candidate, so build them once */
    Vec2 ray_dirs[NN_NUM_VISION_RAYS];
    for (int r = 0; r < NN_NUM_VISION_RAYS; r++) {
        ray_dirs[r] = vec2_from_angle(heading + ray_angles[r]);
    }
    Vec2 forward = vec2_from_angle(heading);

    float ant_dist[NN_NUM_VISION_RAYS];
    float food_dist[NN_NUM_VISION_RAYS];
    float wall_dist[NN_NUM_VISION_RAYS];
    for (int r = 0; r < NN_NUM_VISION_RAYS; r++) {
        ant_dist[r] = range;
        food_dist[r] = range;
        wall_dist[r] = range;
    }

    /* Ants, from the broadphase. The fan only covers what lies ahead, so
       anything behind the ant is rejected with a single dot product. */
    int candidates[MAX_ANT_CANDIDATES];
    int candidate_count = shash_query(&w->ant_grid, pos.x, pos.y, range,
                                     candidates, MAX_ANT_CANDIDATES);
    const float ant_reach_sq = (range + ANT_RADIUS) * (range + ANT_RADIUS);
    for (int i = 0; i < candidate_count; i++) {
        int idx = candidates[i];
        if (idx == self_index) continue;

        Vec2 delta = vec2_sub(w->ants[idx].pos, pos);
        float len_sq = vec2_len_sq(delta);
        if (len_sq > ant_reach_sq) continue;
        if (vec2_dot(delta, forward) < -ANT_RADIUS) continue;

        test_circle_against_rays(delta, len_sq, ANT_RADIUS, ray_dirs, ant_dist);
    }

    /* Food sources are few, so a linear pass is fine */
    const float food_reach_sq = (range + FOOD_RADIUS) * (range + FOOD_RADIUS);
    for (int i = 0; i < w->food_count; i++) {
        if (w->food[i].amount <= 0.0f) continue;

        Vec2 delta = vec2_sub(w->food[i].pos, pos);
        float len_sq = vec2_len_sq(delta);
        if (len_sq > food_reach_sq) continue;
        if (vec2_dot(delta, forward) < -FOOD_RADIUS) continue;

        test_circle_against_rays(delta, len_sq, FOOD_RADIUS, ray_dirs, food_dist);
    }

    /* Walls: walk the grid once, then test each wall against every ray */
    int near_walls[MAX_WALL_CANDIDATES];
    int wall_count = walls_query_circle(&w->walls, pos.x, pos.y, range,
                                       near_walls, MAX_WALL_CANDIDATES);
    for (int i = 0; i < wall_count; i++) {
        for (int r = 0; r < NN_NUM_VISION_RAYS; r++) {
            float d = walls_ray_rect(&w->walls, near_walls[i], pos, ray_dirs[r], wall_dist[r]);
            if (d < wall_dist[r]) wall_dist[r] = d;
        }
    }

    /* Report closeness: 1 = touching, 0 = nothing in range */
    for (int r = 0; r < NN_NUM_VISION_RAYS; r++) {
        out[r].wall = 1.0f - wall_dist[r] / range;
        out[r].ant = 1.0f - ant_dist[r] / range;
        out[r].food = 1.0f - food_dist[r] / range;
    }
}

void vision_to_inputs(const VisionRay rays[NN_NUM_VISION_RAYS], float *inputs) {
    for (int i = 0; i < NN_NUM_VISION_RAYS; i++) {
        inputs[i] = rays[i].wall;
        inputs[NN_NUM_VISION_RAYS + i] = rays[i].ant;
        inputs[2 * NN_NUM_VISION_RAYS + i] = rays[i].food;
    }
}
