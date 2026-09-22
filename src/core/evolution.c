/**
 * @file evolution.c
 * @brief Elite selection, crossover and mutation
 */

#include "evolution.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_WEIGHT_SCALE 0.5f

void evo_init(Evolution *evo) {
    memset(evo, 0, sizeof(*evo));
    evo->generation = 1;
}

void evo_submit(Evolution *evo, const Genome *g, float fitness) {
    evo->gen_sum += fitness;
    evo->gen_evaluated++;
    if (evo->gen_evaluated == 1 || fitness > evo->gen_best) {
        evo->gen_best = fitness;
    }

    if (evo->pool_count < ELITE_MAX) {
        evo->pool[evo->pool_count].genome = *g;
        evo->pool[evo->pool_count].fitness = fitness;
        evo->pool_count++;
        return;
    }

    /* Pool full: replace the weakest if this one is better */
    int worst = 0;
    for (int i = 1; i < evo->pool_count; i++) {
        if (evo->pool[i].fitness < evo->pool[worst].fitness) worst = i;
    }
    if (fitness > evo->pool[worst].fitness) {
        evo->pool[worst].genome = *g;
        evo->pool[worst].fitness = fitness;
    }
}

void evo_breed(const Evolution *evo, const SimConfig *cfg, Genome *out, Rng *rng) {
    if (evo->elite_count == 0) {
        nn_randomize(out, INITIAL_WEIGHT_SCALE, rng);
        return;
    }

    const Genome *a = &evo->elites[rng_int(rng, 0, evo->elite_count)].genome;
    const Genome *b = &evo->elites[rng_int(rng, 0, evo->elite_count)].genome;
    nn_crossover(out, a, b, rng);
    nn_mutate(out, cfg->mutation_rate, cfg->mutation_strength, rng);
}

static int compare_elite_desc(const void *pa, const void *pb) {
    float a = ((const Elite *)pa)->fitness;
    float b = ((const Elite *)pb)->fitness;
    return (a < b) - (a > b);
}

void evo_finish_generation(Evolution *evo, int elite_count) {
    /* Previous parents compete again at reduced fitness (elitism with decay).
       evo_submit would also count them in this generation's stats, so insert
       them into the pool without touching gen_sum / gen_best. */
    float saved_best = evo->gen_best;
    float saved_sum = evo->gen_sum;
    int saved_n = evo->gen_evaluated;
    for (int i = 0; i < evo->elite_count; i++) {
        evo_submit(evo, &evo->elites[i].genome, evo->elites[i].fitness * ELITE_CARRYOVER_DECAY);
    }
    evo->gen_best = saved_best;
    evo->gen_sum = saved_sum;
    evo->gen_evaluated = saved_n;

    qsort(evo->pool, (size_t)evo->pool_count, sizeof(Elite), compare_elite_desc);

    if (elite_count > ELITE_MAX) elite_count = ELITE_MAX;
    evo->elite_count = evo->pool_count < elite_count ? evo->pool_count : elite_count;
    memcpy(evo->elites, evo->pool, sizeof(Elite) * (size_t)evo->elite_count);

    evo->pool_count = 0;
    evo->gen_best = 0.0f;
    evo->gen_sum = 0.0f;
    evo->gen_evaluated = 0;
    evo->timer = 0;
    evo->generation++;
}
