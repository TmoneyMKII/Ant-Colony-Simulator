/**
 * @file utils.h
 * @brief Math helpers and the simulation RNG
 */

#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "types.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PI_F     ((float)M_PI)
#define TWO_PI_F (2.0f * PI_F)

/* ============== Scalar helpers ============== */

static inline float clampf(float x, float min_val, float max_val) {
    return x < min_val ? min_val : (x > max_val ? max_val : x);
}

static inline int clampi(int x, int min_val, int max_val) {
    return x < min_val ? min_val : (x > max_val ? max_val : x);
}

static inline float lerpf(float a, float b, float t) {
    return a + t * (b - a);
}

/** @brief Wrap an angle to [-PI, PI] */
static inline float normalize_angle(float angle) {
    while (angle > PI_F) angle -= TWO_PI_F;
    while (angle < -PI_F) angle += TWO_PI_F;
    return angle;
}

/** @brief Absolute shortest angular distance between two headings */
static inline float angle_diff(float a, float b) {
    return fabsf(normalize_angle(a - b));
}

/** @brief Interpolate between headings along the shortest arc */
static inline float lerp_angle(float from, float to, float t) {
    return normalize_angle(from + normalize_angle(to - from) * t);
}

/* ============== Vec2 ============== */

static inline Vec2 vec2(float x, float y) { return (Vec2){x, y}; }
static inline Vec2 vec2_add(Vec2 a, Vec2 b) { return (Vec2){a.x + b.x, a.y + b.y}; }
static inline Vec2 vec2_sub(Vec2 a, Vec2 b) { return (Vec2){a.x - b.x, a.y - b.y}; }
static inline Vec2 vec2_scale(Vec2 v, float s) { return (Vec2){v.x * s, v.y * s}; }
static inline float vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
static inline float vec2_len_sq(Vec2 v) { return v.x * v.x + v.y * v.y; }
static inline float vec2_len(Vec2 v) { return sqrtf(vec2_len_sq(v)); }
static inline Vec2 vec2_from_angle(float a) { return (Vec2){cosf(a), sinf(a)}; }

static inline float vec2_dist_sq(Vec2 a, Vec2 b) {
    return vec2_len_sq(vec2_sub(a, b));
}

static inline float vec2_dist(Vec2 a, Vec2 b) {
    return sqrtf(vec2_dist_sq(a, b));
}

static inline float vec2_angle_to(Vec2 from, Vec2 to) {
    return atan2f(to.y - from.y, to.x - from.x);
}

/* ============== Rect ============== */

static inline bool rect_contains(const Rect *r, float x, float y) {
    return x >= (float)r->x && x < (float)(r->x + r->width) &&
           y >= (float)r->y && y < (float)(r->y + r->height);
}

/* ============== Random numbers ============== */

/**
 * @brief xorshift32 generator
 *
 * The state is explicit so each World owns its own stream: two worlds
 * created with the same seed and config replay identically even when
 * they are stepped alternately in the same process.
 */
typedef struct {
    uint32_t state;
    bool has_spare;     /**< Gaussian pairs: a value is held over */
    float spare;
} Rng;

/** @brief Seed a generator (seed 0 is replaced; xorshift needs non-zero state) */
void rng_init(Rng *rng, uint32_t seed);

/** @brief Raw 32-bit draw */
uint32_t rng_next(Rng *rng);

/** @brief Uniform float in [0, 1) */
float rng_float(Rng *rng);

/** @brief Uniform float in [min, max) */
static inline float rng_range(Rng *rng, float min_val, float max_val) {
    return min_val + rng_float(rng) * (max_val - min_val);
}

/** @brief Uniform int in [min, max) */
int rng_int(Rng *rng, int min_val, int max_val);

/** @brief Normally distributed float */
float rng_gaussian(Rng *rng, float mean, float stddev);

#endif /* UTILS_H */
