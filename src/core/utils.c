/**
 * @file utils.c
 * @brief RNG implementation
 */

#include "utils.h"

void rng_init(Rng *rng, uint32_t seed) {
    rng->state = seed ? seed : 0x12345678u;
    rng->has_spare = false;
    rng->spare = 0.0f;
}

uint32_t rng_next(Rng *rng) {
    uint32_t x = rng->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

float rng_float(Rng *rng) {
    /* Top 24 bits -> exactly representable float in [0, 1) */
    return (float)(rng_next(rng) >> 8) * (1.0f / 16777216.0f);
}

int rng_int(Rng *rng, int min_val, int max_val) {
    if (min_val >= max_val) return min_val;
    return min_val + (int)(rng_next(rng) % (uint32_t)(max_val - min_val));
}

/** @brief Marsaglia polar method; generates values in pairs */
float rng_gaussian(Rng *rng, float mean, float stddev) {
    if (rng->has_spare) {
        rng->has_spare = false;
        return mean + stddev * rng->spare;
    }

    float u, v, s;
    do {
        u = rng_float(rng) * 2.0f - 1.0f;
        v = rng_float(rng) * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);

    s = sqrtf(-2.0f * logf(s) / s);
    rng->spare = v * s;
    rng->has_spare = true;

    return mean + stddev * u * s;
}
