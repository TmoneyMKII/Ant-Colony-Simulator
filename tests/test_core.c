/**
 * @file test_core.c
 * @brief Tests for the simulation core (no SDL, no rendering)
 *
 * Run with: ctest --test-dir build   (or just ./antsim_tests)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "world.h"
#include "utils.h"
#include "spatial_hash.h"
#include "walls.h"
#include "pheromone.h"
#include "neural_net.h"

static int checks_run = 0;
static int failures = 0;
static const char *current_test = "";

#define CHECK(cond) do {                                                      \
        checks_run++;                                                         \
        if (!(cond)) {                                                        \
            failures++;                                                       \
            printf("  FAIL %s:%d in %s: %s\n", __FILE__, __LINE__,            \
                   current_test, #cond);                                      \
        }                                                                     \
    } while (0)

#define CHECK_NEAR(a, b, tol) CHECK(fabsf((float)(a) - (float)(b)) <= (tol))

#define RUN(fn) do {                                                          \
        current_test = #fn;                                                   \
        int before = failures;                                                \
        fn();                                                                 \
        printf("%-34s %s\n", #fn, failures == before ? "ok" : "FAILED");      \
    } while (0)

/* ============== Spatial hash ============== */

static void test_spatial_hash_matches_brute_force(void) {
    SpatialHash h;
    CHECK(shash_init(&h, 800, 600, 100, 1000));

    Rng rng;
    rng_init(&rng, 7);
    Vec2 points[400];
    for (int i = 0; i < 400; i++) {
        points[i] = vec2(rng_range(&rng, 0, 800), rng_range(&rng, 0, 600));
    }
    shash_build(&h, points, 400);

    /* Every point must appear exactly once in the grid */
    int seen[400] = {0};
    int total = 0;
    for (int c = 0; c < h.cols * h.rows; c++) {
        for (int k = h.cell_start[c]; k < h.cell_start[c + 1]; k++) {
            seen[h.items[k]]++;
            total++;
        }
    }
    CHECK(total == 400);
    for (int i = 0; i < 400; i++) CHECK(seen[i] == 1);

    /* A query must return a superset of the points truly inside the radius */
    const float radius = 60.0f;
    for (int q = 0; q < 20; q++) {
        Vec2 center = vec2(rng_range(&rng, 0, 800), rng_range(&rng, 0, 600));
        int out[1000];
        int n = shash_query(&h, center.x, center.y, radius, out, 1000);

        for (int i = 0; i < 400; i++) {
            if (vec2_dist(center, points[i]) > radius) continue;
            bool found = false;
            for (int k = 0; k < n; k++) {
                if (out[k] == i) { found = true; break; }
            }
            CHECK(found);
        }
    }

    shash_free(&h);
}

/* ============== Walls ============== */

static void test_wall_spanning_many_cells_is_found(void) {
    /* Regression: a wall covering several broadphase cells used to be
       reachable from only one of them, letting ants walk through it. */
    WallSet ws;
    CHECK(walls_init(&ws, 1000, 1000));

    Rect wide = {100, 300, 600, 40};  /* spans ~10 cells horizontally */
    CHECK(walls_add(&ws, wide));
    walls_add(&ws, (Rect){800, 800, 30, 30});
    walls_rebuild(&ws);

    for (int x = 105; x < 700; x += 5) {
        CHECK(walls_overlaps_circle(&ws, (float)x, 320.0f, 1.0f));
    }
    CHECK(!walls_overlaps_circle(&ws, 50.0f, 320.0f, 1.0f));
    CHECK(!walls_overlaps_circle(&ws, 400.0f, 200.0f, 1.0f));
    CHECK(walls_overlaps_circle(&ws, 810.0f, 810.0f, 1.0f));

    walls_free(&ws);
}

static void test_wall_resolve_pushes_circle_out(void) {
    WallSet ws;
    CHECK(walls_init(&ws, 1000, 1000));
    walls_add(&ws, (Rect){400, 400, 200, 200});
    walls_rebuild(&ws);

    /* Overlapping an edge: pushed out along the shortest direction */
    Vec2 pos = vec2(395.0f, 500.0f);
    Vec2 normal = {0, 0};
    CHECK(walls_resolve_circle(&ws, &pos, 10.0f, &normal));
    CHECK(pos.x <= 390.0f);
    CHECK_NEAR(normal.x, -1.0f, 0.001f);
    CHECK(!walls_overlaps_circle(&ws, pos.x, pos.y, 10.0f));

    /* Deep inside: still ends up outside the wall */
    pos = vec2(450.0f, 450.0f);
    CHECK(walls_resolve_circle(&ws, &pos, 6.0f, &normal));
    CHECK(!walls_overlaps_circle(&ws, pos.x, pos.y, 6.0f));

    /* Clear of the wall: untouched */
    pos = vec2(100.0f, 100.0f);
    CHECK(!walls_resolve_circle(&ws, &pos, 6.0f, &normal));
    CHECK_NEAR(pos.x, 100.0f, 0.0001f);

    walls_free(&ws);
}

static void test_wall_raycast_distance(void) {
    WallSet ws;
    CHECK(walls_init(&ws, 1000, 1000));
    walls_add(&ws, (Rect){500, 0, 20, 1000});
    walls_rebuild(&ws);

    float d = walls_raycast(&ws, vec2(100, 500), vec2(1, 0), 1000.0f);
    CHECK_NEAR(d, 400.0f, 0.01f);

    /* Pointing away: no hit */
    d = walls_raycast(&ws, vec2(100, 500), vec2(-1, 0), 1000.0f);
    CHECK_NEAR(d, 1000.0f, 0.01f);

    /* Out of reach: clamped to max */
    d = walls_raycast(&ws, vec2(100, 500), vec2(1, 0), 200.0f);
    CHECK_NEAR(d, 200.0f, 0.01f);

    /* Diagonal hit, 45 degrees: distance = 400 * sqrt(2) */
    d = walls_raycast(&ws, vec2(100, 100), vec2(0.70710678f, 0.70710678f), 1000.0f);
    CHECK_NEAR(d, 565.685f, 0.5f);

    walls_free(&ws);
}

/* ============== Pheromones ============== */

static void test_pheromone_deposit_sample_evaporate(void) {
    PheromoneMap map;
    CHECK(pheromone_init(&map, 400, 400, 20));

    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 50, 50), 0.0f, 0.0001f);

    pheromone_deposit(&map, PHEROMONE_FOOD, 50, 50, 30.0f);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 50, 50), 30.0f, 0.001f);
    /* Same cell (20px cells), so the value is shared */
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 55, 55), 30.0f, 0.001f);
    /* Layers are independent */
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_HOME, 50, 50), 0.0f, 0.001f);

    /* Clamped at the maximum */
    pheromone_deposit(&map, PHEROMONE_FOOD, 50, 50, 10000.0f);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 50, 50), PHEROMONE_MAX_VALUE, 0.001f);

    /* Danger decays at its own rate */
    pheromone_deposit(&map, PHEROMONE_DANGER, 50, 50, 100.0f);
    pheromone_evaporate(&map, 0.5f, 0.9f);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 50, 50), PHEROMONE_MAX_VALUE * 0.5f, 0.01f);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_DANGER, 50, 50), 90.0f, 0.01f);

    /* Out of bounds is silent and reads zero */
    pheromone_deposit(&map, PHEROMONE_FOOD, -10, -10, 50.0f);
    pheromone_deposit(&map, PHEROMONE_FOOD, 9999, 9999, 50.0f);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, -10, -10), 0.0f, 0.001f);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 9999, 9999), 0.0f, 0.001f);

    pheromone_clear(&map);
    CHECK_NEAR(pheromone_sample(&map, PHEROMONE_FOOD, 50, 50), 0.0f, 0.001f);

    pheromone_free(&map);
}

static void test_pheromone_steer_points_at_strongest_neighbour(void) {
    PheromoneMap map;
    CHECK(pheromone_init(&map, 400, 400, 20));

    float dir = 0.0f;
    /* Nothing deposited, and below-threshold amounts, give no direction */
    CHECK(!pheromone_steer(&map, PHEROMONE_FOOD, 200, 200, 0.0f, &dir));
    pheromone_deposit(&map, PHEROMONE_FOOD, 220, 200, PHEROMONE_DETECT_THRESHOLD * 0.5f);
    CHECK(!pheromone_steer(&map, PHEROMONE_FOOD, 200, 200, 0.0f, &dir));

    /* A strong cell to the right (+x) is steered toward */
    pheromone_deposit(&map, PHEROMONE_FOOD, 220, 200, 100.0f);
    CHECK(pheromone_steer(&map, PHEROMONE_FOOD, 200, 200, 0.0f, &dir));
    CHECK_NEAR(dir, 0.0f, 0.001f);

    /* A stronger cell below (+y) wins when the ant is heading that way */
    pheromone_deposit(&map, PHEROMONE_FOOD, 200, 220, 200.0f);
    CHECK(pheromone_steer(&map, PHEROMONE_FOOD, 200, 200, PI_F * 0.5f, &dir));
    CHECK_NEAR(dir, PI_F * 0.5f, 0.001f);

    pheromone_free(&map);
}

/* ============== Neural network ============== */

static void test_nn_forward_and_genetics(void) {
    Rng rng;
    rng_init(&rng, 3);

    Genome a, b;
    nn_randomize(&a, 0.5f, &rng);
    nn_randomize(&b, 0.5f, &rng);

    float inputs[NN_INPUT_SIZE];
    for (int i = 0; i < NN_INPUT_SIZE; i++) inputs[i] = (float)i / NN_INPUT_SIZE;

    NNActivations act1, act2;
    nn_forward(&a, inputs, &act1);
    nn_forward(&a, inputs, &act2);

    /* Deterministic, bounded by tanh, and inputs are recorded for the UI */
    for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
        CHECK_NEAR(act1.outputs[k], act2.outputs[k], 0.0f);
        CHECK(act1.outputs[k] >= -1.0f && act1.outputs[k] <= 1.0f);
    }
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        CHECK(act1.hidden[j] >= -1.0f && act1.hidden[j] <= 1.0f);
    }
    CHECK_NEAR(act1.inputs[5], inputs[5], 0.0f);

    /* Crossover only ever copies weights from a parent */
    Genome child;
    nn_crossover(&child, &a, &b, &rng);
    for (int i = 0; i < NN_WEIGHT_COUNT; i++) {
        CHECK(child.w[i] == a.w[i] || child.w[i] == b.w[i]);
    }

    /* Mutation with rate 0 changes nothing; with rate 1 it changes everything */
    Genome copy = child;
    nn_mutate(&copy, 0.0f, 1.0f, &rng);
    CHECK(memcmp(&copy, &child, sizeof(Genome)) == 0);
    nn_mutate(&copy, 1.0f, 0.5f, &rng);
    CHECK(memcmp(&copy, &child, sizeof(Genome)) != 0);

    /* Different weights give different behaviour */
    nn_forward(&b, inputs, &act2);
    bool differs = false;
    for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
        if (fabsf(act1.outputs[k] - act2.outputs[k]) > 1e-6f) differs = true;
    }
    CHECK(differs);
}

/* ============== Evolution ============== */

static void test_evolution_keeps_the_best(void) {
    Rng rng;
    rng_init(&rng, 11);
    Evolution evo;
    evo_init(&evo);
    CHECK(evo.generation == 1);
    CHECK(evo.elite_count == 0);

    SimConfig cfg;
    sim_config_defaults(&cfg);

    /* With no elites yet, breeding still produces a usable genome */
    Genome g;
    evo_breed(&evo, &cfg, &g, &rng);

    /* Submit many genomes with increasing fitness */
    for (int i = 0; i < 100; i++) {
        Genome cand;
        nn_randomize(&cand, 0.5f, &rng);
        evo_submit(&evo, &cand, (float)i);
    }
    CHECK(evo.gen_best == 99.0f);
    CHECK(evo.gen_evaluated == 100);
    CHECK(evo.pool_count == ELITE_MAX);

    evo_finish_generation(&evo, 10);
    CHECK(evo.generation == 2);
    CHECK(evo.elite_count == 10);
    CHECK(evo.timer == 0);
    /* Elites are sorted best first and hold the top scores submitted */
    CHECK(evo.elites[0].fitness == 99.0f);
    for (int i = 1; i < evo.elite_count; i++) {
        CHECK(evo.elites[i - 1].fitness >= evo.elites[i].fitness);
    }
    /* Generation stats were reset for the new generation */
    CHECK(evo.gen_evaluated == 0);
    CHECK(evo.pool_count == 0);

    /* A generation full of poor genomes cannot erase the earlier best */
    for (int i = 0; i < 5; i++) {
        Genome cand;
        nn_randomize(&cand, 0.5f, &rng);
        evo_submit(&evo, &cand, 1.0f);
    }
    evo_finish_generation(&evo, 10);
    CHECK(evo.elites[0].fitness > 50.0f);
}

/* ============== World ============== */

static World *make_world(uint32_t seed, int ants) {
    SimConfig cfg;
    sim_config_defaults(&cfg);
    cfg.population = ants;
    return world_create(&cfg, seed);
}

static void test_world_setup(void) {
    World *w = make_world(5, 50);
    CHECK(w != NULL);
    if (!w) return;

    CHECK(w->ant_count == 50);
    CHECK(w->tick == 0);
    CHECK(w->food_count > 0);
    CHECK(w->walls.count > 0);
    CHECK(w->nest.food_stored == 0.0f);
    CHECK_NEAR(w->nest.pos.x, w->width / 2, 1.0f);

    /* Ants start inside the nest and every ant has a distinct id */
    for (int i = 0; i < w->ant_count; i++) {
        CHECK(vec2_dist(w->ants[i].pos, w->nest.pos) <= w->nest.radius);
        for (int j = i + 1; j < w->ant_count; j++) {
            CHECK(w->ants[i].id != w->ants[j].id);
        }
    }

    /* Food and ants never spawn inside walls */
    for (int i = 0; i < w->food_count; i++) {
        CHECK(!walls_overlaps_circle(&w->walls, w->food[i].pos.x, w->food[i].pos.y, 1.0f));
    }

    world_destroy(w);
}

static void test_world_step_keeps_ants_legal(void) {
    World *w = make_world(9, 120);
    CHECK(w != NULL);
    if (!w) return;

    for (int t = 0; t < 400; t++) world_step(w);

    CHECK(w->tick == 400);
    CHECK(w->ant_count == 120);   /* deaths respawn, so population holds */

    for (int i = 0; i < w->ant_count; i++) {
        const Ant *a = &w->ants[i];
        CHECK(a->pos.x >= 0.0f && a->pos.x <= (float)w->width);
        CHECK(a->pos.y >= 0.0f && a->pos.y <= (float)w->height);
        CHECK(a->energy > 0.0f && a->energy <= ANT_MAX_ENERGY);
        CHECK(!walls_overlaps_circle(&w->walls, a->pos.x, a->pos.y, ANT_RADIUS * 0.5f));
        CHECK(a->heading >= -PI_F - 0.01f && a->heading <= PI_F + 0.01f);
    }

    /* Trails exist where ants have walked */
    float trail_total = 0.0f;
    const float *home = pheromone_layer(&w->pheromones, PHEROMONE_HOME);
    for (int i = 0; i < w->pheromones.cols * w->pheromones.rows; i++) trail_total += home[i];
    CHECK(trail_total > 0.0f);

    world_destroy(w);
}

static void test_world_is_deterministic_for_a_seed(void) {
    World *a = make_world(1234, 80);
    World *b = make_world(1234, 80);
    CHECK(a && b);
    if (!a || !b) return;

    for (int t = 0; t < 250; t++) {
        world_step(a);
        world_step(b);
    }

    CHECK(a->walls.count == b->walls.count);
    CHECK(a->food_count == b->food_count);
    CHECK(a->stats.total_deliveries == b->stats.total_deliveries);
    CHECK_NEAR(a->nest.food_stored, b->nest.food_stored, 0.0f);
    for (int i = 0; i < a->ant_count; i++) {
        CHECK_NEAR(a->ants[i].pos.x, b->ants[i].pos.x, 0.0f);
        CHECK_NEAR(a->ants[i].pos.y, b->ants[i].pos.y, 0.0f);
        CHECK(a->ants[i].id == b->ants[i].id);
    }

    /* A different seed diverges */
    World *c = make_world(4321, 80);
    CHECK(c != NULL);
    if (c) {
        for (int t = 0; t < 250; t++) world_step(c);
        bool same = fabsf(c->ants[0].pos.x - a->ants[0].pos.x) < 0.0001f &&
                    fabsf(c->ants[0].pos.y - a->ants[0].pos.y) < 0.0001f;
        CHECK(!same);
        world_destroy(c);
    }

    world_destroy(a);
    world_destroy(b);
}

static void test_world_food_and_population_controls(void) {
    World *w = make_world(17, 10);
    CHECK(w != NULL);
    if (!w) return;

    /* Food cannot be placed in a wall or outside the world */
    int before = w->food_count;
    Rect wall = w->walls.count > 0 ? w->walls.rects[0] : (Rect){0, 0, 0, 0};
    if (wall.width > 0) {
        float cx = (float)wall.x + (float)wall.width * 0.5f;
        float cy = (float)wall.y + (float)wall.height * 0.5f;
        CHECK(!world_add_food(w, cx, cy, 50.0f));
    }
    CHECK(!world_add_food(w, -5.0f, 50.0f, 50.0f));
    CHECK(!world_add_food(w, (float)w->width + 10.0f, 50.0f, 50.0f));
    CHECK(w->food_count == before);

    /* A clear spot works and holds the requested amount */
    Vec2 spot = vec2(w->nest.pos.x + 60.0f, w->nest.pos.y);
    if (!walls_overlaps_circle(&w->walls, spot.x, spot.y, FOOD_RADIUS)) {
        CHECK(world_add_food(w, spot.x, spot.y, 42.0f));
        CHECK(w->food_count == before + 1);
        CHECK_NEAR(w->food[w->food_count - 1].amount, 42.0f, 0.001f);
    }

    /* Taking food never goes below zero */
    int idx = w->food_count - 1;
    w->food[idx].amount = 0.4f;
    CHECK_NEAR(world_take_food(w, idx), 0.4f, 0.001f);
    CHECK_NEAR(w->food[idx].amount, 0.0f, 0.001f);
    CHECK_NEAR(world_take_food(w, idx), 0.0f, 0.001f);
    CHECK_NEAR(world_take_food(w, 9999), 0.0f, 0.001f);

    /* Population slider grows and shrinks the colony */
    w->cfg.population = 40;
    world_apply_population(w);
    CHECK(w->ant_count == 40);
    w->cfg.population = 5;
    world_apply_population(w);
    CHECK(w->ant_count == 5);

    /* Lookup by position and by id */
    Ant *target = &w->ants[2];
    int found = world_ant_at(w, target->pos.x, target->pos.y, 5.0f);
    CHECK(found == 2);
    CHECK(world_ant_by_id(w, target->id) == 2);
    CHECK(world_ant_by_id(w, 999999) == -1);
    CHECK(world_ant_at(w, -500.0f, -500.0f, 5.0f) == -1);

    world_destroy(w);
}

static void test_world_generations_and_stats(void) {
    SimConfig cfg;
    sim_config_defaults(&cfg);
    cfg.population = 30;
    cfg.generation_ticks = 60;      /* short generations to exercise the path */
    cfg.replace_fraction = 0.5f;

    World *w = world_create(&cfg, 21);
    CHECK(w != NULL);
    if (!w) return;

    for (int t = 0; t < 300; t++) world_step(w);

    CHECK(w->evo.generation == 6);          /* 300 / 60 generations elapsed */
    CHECK(w->evo.elite_count > 0);          /* living ants were evaluated */
    CHECK(w->stats.gen_count == 5);
    CHECK(w->ant_count == 30);              /* replacement keeps the size */

    const GenerationStats *g = stats_generation(&w->stats, 0);
    CHECK(g != NULL);
    if (g) {
        CHECK(g->generation == 1);
        CHECK(g->evaluated >= 30);
        CHECK(g->avg_fitness > 0.0f);       /* survival alone scores */
    }
    /* History is ordered oldest to newest */
    const GenerationStats *last = stats_generation(&w->stats, w->stats.gen_count - 1);
    CHECK(last && last->generation == 5);
    CHECK(stats_generation(&w->stats, w->stats.gen_count) == NULL);

    /* Samples were taken every STATS_SAMPLE_INTERVAL ticks */
    CHECK(w->stats.sample_count == 300 / STATS_SAMPLE_INTERVAL);
    CHECK(stats_sample(&w->stats, 0) != NULL);
    CHECK(stats_sample(&w->stats, -1) == NULL);

    world_destroy(w);
}

static void test_world_reset_restores_start_state(void) {
    World *w = make_world(3, 25);
    CHECK(w != NULL);
    if (!w) return;

    for (int t = 0; t < 200; t++) world_step(w);
    w->nest.food_stored = 500.0f;

    world_reset(w, 3);
    CHECK(w->tick == 0);
    CHECK(w->nest.food_stored == 0.0f);
    CHECK(w->evo.generation == 1);
    CHECK(w->marker_count == 0);
    CHECK(w->stats.total_deliveries == 0);
    CHECK(w->stats.sample_count == 0);
    CHECK(w->ant_count == 25);

    /* Resetting with a new world size reallocates the grids */
    w->cfg.world_width = 800;
    w->cfg.world_height = 600;
    world_reset(w, 3);
    CHECK(w->width == 800);
    CHECK(w->pheromones.cols == 800 / PHEROMONE_CELL_SIZE);
    for (int t = 0; t < 60; t++) world_step(w);
    for (int i = 0; i < w->ant_count; i++) {
        CHECK(w->ants[i].pos.x <= 800.0f && w->ants[i].pos.y <= 600.0f);
    }

    world_destroy(w);
}

static void test_classic_brain_runs(void) {
    SimConfig cfg;
    sim_config_defaults(&cfg);
    cfg.brain_mode = BRAIN_CLASSIC;
    cfg.population = 40;

    World *w = world_create(&cfg, 8);
    CHECK(w != NULL);
    if (!w) return;

    for (int t = 0; t < 300; t++) world_step(w);
    CHECK(w->ant_count == 40);
    for (int i = 0; i < w->ant_count; i++) {
        CHECK(w->ants[i].pos.x >= 0.0f && w->ants[i].pos.x <= (float)w->width);
    }

    world_destroy(w);
}

static void test_config_sanitize_clamps(void) {
    SimConfig cfg;
    sim_config_defaults(&cfg);

    cfg.population = 999999;
    cfg.elite_count = 999;
    cfg.mutation_rate = 5.0f;
    cfg.trail_evaporation = 2.0f;
    cfg.deposit_interval = 0;
    cfg.food_max = 1.0f;
    cfg.food_min = 500.0f;
    sim_config_sanitize(&cfg);

    CHECK(cfg.population == MAX_POPULATION);
    CHECK(cfg.elite_count == ELITE_MAX);
    CHECK(cfg.mutation_rate == 1.0f);
    CHECK(cfg.trail_evaporation == 1.0f);
    CHECK(cfg.deposit_interval == 1);
    CHECK(cfg.food_max >= cfg.food_min);
}

/* ============== Entry point ============== */

int main(void) {
    printf("ant simulator core tests\n\n");

    RUN(test_spatial_hash_matches_brute_force);
    RUN(test_wall_spanning_many_cells_is_found);
    RUN(test_wall_resolve_pushes_circle_out);
    RUN(test_wall_raycast_distance);
    RUN(test_pheromone_deposit_sample_evaporate);
    RUN(test_pheromone_steer_points_at_strongest_neighbour);
    RUN(test_nn_forward_and_genetics);
    RUN(test_evolution_keeps_the_best);
    RUN(test_world_setup);
    RUN(test_world_step_keeps_ants_legal);
    RUN(test_world_is_deterministic_for_a_seed);
    RUN(test_world_food_and_population_controls);
    RUN(test_world_generations_and_stats);
    RUN(test_world_reset_restores_start_state);
    RUN(test_classic_brain_runs);
    RUN(test_config_sanitize_clamps);

    printf("\n%d checks, %d failures\n", checks_run, failures);
    return failures == 0 ? 0 : 1;
}
