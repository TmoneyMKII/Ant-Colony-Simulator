/**
 * @file evolution.h
 * @brief Generational genetic algorithm over ant brains
 *
 * Each generation, every ant that dies and every ant still alive at the
 * end of the generation is scored. The best genomes (plus the previous
 * elites at a decayed fitness, so a bad generation cannot erase them)
 * become the parents for every ant born in the next generation.
 */

#ifndef EVOLUTION_H
#define EVOLUTION_H

#include "neural_net.h"
#include "sim_config.h"

typedef struct {
    Genome genome;
    float fitness;
} Elite;

typedef struct {
    Elite elites[ELITE_MAX];    /**< Parents, best first */
    int elite_count;

    Elite pool[ELITE_MAX];      /**< Best candidates seen this generation (unsorted) */
    int pool_count;

    int generation;             /**< 1-based */
    int timer;                  /**< Ticks into the current generation */

    /* Scores recorded this generation */
    float gen_best;
    float gen_sum;
    int gen_evaluated;
} Evolution;

void evo_init(Evolution *evo);

/** @brief Record one evaluated genome for the current generation */
void evo_submit(Evolution *evo, const Genome *g, float fitness);

/** @brief Produce a child from two random elites (random genome if none yet) */
void evo_breed(const Evolution *evo, const SimConfig *cfg, Genome *out, Rng *rng);

/** @brief Promote this generation's best to elites and start the next one */
void evo_finish_generation(Evolution *evo, int elite_count);

#endif /* EVOLUTION_H */
