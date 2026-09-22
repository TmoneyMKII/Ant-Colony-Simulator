/**
 * @file utils.c
 * @brief Utility function implementations
 */

#include "utils.h"
#include <time.h>

/* ============== XorShift RNG ============== */
static uint32_t rng_state = 0x12345678;

void rng_seed(uint32_t seed) {
    rng_state = seed ? seed : (uint32_t)time(NULL);
}

/**
 * @brief XorShift32 random number generator
 */
static uint32_t xorshift32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

float randf(void) {
    return (float)xorshift32() / (float)UINT32_MAX;
}

int randi_range(int min_val, int max_val) {
    if (min_val >= max_val) return min_val;
    return min_val + (int)(xorshift32() % (uint32_t)(max_val - min_val));
}

/**
 * @brief Box-Muller transform for Gaussian distribution
 */
float rand_gaussian(float mean, float stddev) {
    static int has_spare = 0;
    static float spare;
    
    if (has_spare) {
        has_spare = 0;
        return mean + stddev * spare;
    }
    
    has_spare = 1;
    
    float u, v, s;
    do {
        u = randf() * 2.0f - 1.0f;
        v = randf() * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    s = sqrtf(-2.0f * logf(s) / s);
    spare = v * s;
    
    return mean + stddev * u * s;
}
