/**
 * @file utils.h
 * @brief Utility functions and math helpers
 */

#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include "types.h"

/* ============== Constants ============== */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0 * M_PI)

/* ============== Math Utilities ============== */

/**
 * @brief Clamp a value to a range
 */
static inline float clampf(float x, float min_val, float max_val) {
    return x < min_val ? min_val : (x > max_val ? max_val : x);
}

static inline int clampi(int x, int min_val, int max_val) {
    return x < min_val ? min_val : (x > max_val ? max_val : x);
}

/**
 * @brief Linear interpolation
 */
static inline float lerpf(float a, float b, float t) {
    return a + t * (b - a);
}

/**
 * @brief Squared distance between two points (avoids sqrt)
 */
static inline float dist_sq(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

/**
 * @brief Distance between two points
 */
static inline float dist(float x1, float y1, float x2, float y2) {
    return sqrtf(dist_sq(x1, y1, x2, y2));
}

/**
 * @brief Normalize an angle to [-PI, PI]
 */
static inline float normalize_angle(float angle) {
    while (angle > M_PI) angle -= TWO_PI;
    while (angle < -M_PI) angle += TWO_PI;
    return angle;
}

/**
 * @brief Angle difference (shortest path)
 */
static inline float angle_diff(float a, float b) {
    float diff = normalize_angle(a - b);
    return fabsf(diff);
}

/**
 * @brief Vector length
 */
static inline float vec2_len(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

/**
 * @brief Normalize vector
 */
static inline Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_len(v);
    if (len < 0.0001f) return (Vec2){0, 0};
    return (Vec2){v.x / len, v.y / len};
}

/**
 * @brief Dot product
 */
static inline float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

/* ============== Random Number Generation ============== */

/**
 * @brief Initialize RNG with seed
 */
void rng_seed(uint32_t seed);

/**
 * @brief Get random float in [0, 1]
 */
float randf(void);

/**
 * @brief Get random float in [min, max]
 */
static inline float randf_range(float min_val, float max_val) {
    return min_val + randf() * (max_val - min_val);
}

/**
 * @brief Get random int in [min, max)
 */
int randi_range(int min_val, int max_val);

/**
 * @brief Get random value from Gaussian distribution
 */
float rand_gaussian(float mean, float stddev);

/* ============== Activation Functions ============== */

/**
 * @brief Sigmoid activation (0 to 1)
 */
static inline float sigmoid(float x) {
    x = clampf(x, -500.0f, 500.0f);
    return 1.0f / (1.0f + expf(-x));
}

/**
 * @brief Tanh activation (-1 to 1)
 */
static inline float fast_tanh(float x) {
    x = clampf(x, -500.0f, 500.0f);
    return tanhf(x);
}

/**
 * @brief ReLU activation
 */
static inline float relu(float x) {
    return x > 0 ? x : 0;
}

/* ============== Rectangle Utilities ============== */

/**
 * @brief Check if point is inside rectangle
 */
static inline bool rect_contains(const Rect *r, int x, int y) {
    return x >= r->x && x < r->x + r->width &&
           y >= r->y && y < r->y + r->height;
}

/**
 * @brief Check if rectangles intersect
 */
static inline bool rect_intersects(const Rect *a, const Rect *b) {
    return a->x < b->x + b->width && a->x + a->width > b->x &&
           a->y < b->y + b->height && a->y + a->height > b->y;
}

#endif /* UTILS_H */
