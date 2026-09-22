/**
 * @file headless.c
 * @brief Run the simulation with no window: fast evolution and benchmarking
 *
 *   antsim_headless [options]
 *     --ticks N        simulation ticks to run (default 3600, 0 = forever)
 *     --seed N         world seed (default 1)
 *     --ants N         population (default from config)
 *     --report N       ticks between report lines (default 600)
 *     --size WxH       world size (default 1920x1080)
 *     --classic        use the hand-written brain instead of the network
 *     --bench          print timing only
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "world.h"
#include "utils.h"

static void print_usage(void) {
    printf("usage: antsim_headless [--ticks N] [--seed N] [--ants N] [--report N]\n"
           "                       [--size WxH] [--classic] [--bench]\n");
}

int main(int argc, char **argv) {
    SimConfig cfg;
    sim_config_defaults(&cfg);

    long ticks = 3600;
    long report_every = 600;
    uint32_t seed = 1;
    bool bench = false;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        bool has_value = (i + 1 < argc);

        if (!strcmp(arg, "--ticks") && has_value) {
            ticks = atol(argv[++i]);
        } else if (!strcmp(arg, "--seed") && has_value) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (!strcmp(arg, "--ants") && has_value) {
            cfg.population = atoi(argv[++i]);
        } else if (!strcmp(arg, "--report") && has_value) {
            report_every = atol(argv[++i]);
        } else if (!strcmp(arg, "--size") && has_value) {
            int w, h;
            if (sscanf(argv[++i], "%dx%d", &w, &h) == 2) {
                cfg.world_width = w;
                cfg.world_height = h;
            }
        } else if (!strcmp(arg, "--classic")) {
            cfg.brain_mode = BRAIN_CLASSIC;
        } else if (!strcmp(arg, "--bench")) {
            bench = true;
        } else {
            print_usage();
            return arg[0] == '-' ? 1 : 0;
        }
    }

    World *w = world_create(&cfg, seed);
    if (!w) {
        fprintf(stderr, "failed to create world\n");
        return 1;
    }

    printf("world %dx%d  seed %u  ants %d  brain %s  walls %d  food %d\n",
           w->width, w->height, w->seed, w->ant_count,
           w->cfg.brain_mode == BRAIN_NEURAL ? "neural" : "classic",
           w->walls.count, w->food_count);

    if (!bench) {
        printf("%10s %8s %8s %8s %6s %10s %10s\n",
               "tick", "food", "delivs", "carrying", "gen", "best fit", "deaths");
    }

    clock_t start = clock();
    for (long t = 0; ticks == 0 || t < ticks; t++) {
        world_step(w);

        if (!bench && report_every > 0 && w->tick % (uint64_t)report_every == 0) {
            float best = w->evo.elite_count > 0 ? w->evo.elites[0].fitness : 0.0f;
            printf("%10llu %8.0f %8llu %8d %6d %10.1f %10llu\n",
                   (unsigned long long)w->tick, (double)w->nest.food_stored,
                   (unsigned long long)w->stats.total_deliveries, w->stats.carrying,
                   w->evo.generation, (double)best,
                   (unsigned long long)(w->stats.deaths_starved + w->stats.deaths_stuck));
            fflush(stdout);
        }
    }
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;

    printf("\n%ld ticks in %.2fs = %.0f ticks/s (%.3f ms/tick) with %d ants\n",
           ticks, elapsed, ticks / elapsed, elapsed * 1000.0 / (double)ticks, w->ant_count);
    printf("food delivered %llu, deaths %llu starved / %llu stuck, generation %d\n",
           (unsigned long long)w->stats.total_deliveries,
           (unsigned long long)w->stats.deaths_starved,
           (unsigned long long)w->stats.deaths_stuck,
           w->evo.generation);

    world_destroy(w);
    return 0;
}
