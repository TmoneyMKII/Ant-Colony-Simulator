/**
 * @file world.c
 * @brief World lifetime, the step function, and colony-level bookkeeping
 */

#include "world.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_FOOD_CAPACITY  32
#define INITIAL_EVENT_CAPACITY 64
#define PLACEMENT_ATTEMPTS     24
#define NEST_KEEP_CLEAR        (NEST_RADIUS * 3.0f)

/* ============== Events ============== */

static void emit(World *w, SimEventType type, uint32_t ant_id, Vec2 pos, int detail) {
    if (w->event_count >= w->event_capacity) {
        int new_cap = w->event_capacity ? w->event_capacity * 2 : INITIAL_EVENT_CAPACITY;
        SimEvent *grown = realloc(w->events, sizeof(SimEvent) * (size_t)new_cap);
        if (!grown) return;  /* dropping an event is better than failing the step */
        w->events = grown;
        w->event_capacity = new_cap;
    }
    w->events[w->event_count++] = (SimEvent){type, ant_id, pos, detail};
}

/* ============== Placement helpers ============== */

/** @brief Random spot inside the world that is not inside a wall */
static bool random_free_position(World *w, float clearance, Vec2 *out) {
    float x_min = FOOD_EDGE_MARGIN;
    float x_max = (float)w->width - FOOD_EDGE_MARGIN;
    float y_min = FOOD_EDGE_MARGIN;
    float y_max = (float)w->height - FOOD_EDGE_MARGIN;

    for (int attempt = 0; attempt < PLACEMENT_ATTEMPTS; attempt++) {
        Vec2 p = vec2(rng_range(&w->rng, x_min, x_max), rng_range(&w->rng, y_min, y_max));
        if (!walls_overlaps_circle(&w->walls, p.x, p.y, clearance)) {
            *out = p;
            return true;
        }
    }
    return false;
}

static void spawn_food_sources(World *w) {
    w->food_count = 0;
    for (int i = 0; i < w->cfg.food_source_count; i++) {
        Vec2 p;
        if (!random_free_position(w, FOOD_RADIUS * 2.0f, &p)) continue;
        world_add_food(w, p.x, p.y, rng_range(&w->rng, w->cfg.food_min, w->cfg.food_max));
    }
}

/** @brief Random point just inside the nest */
static Vec2 nest_spawn_point(World *w) {
    float angle = rng_range(&w->rng, 0.0f, TWO_PI_F);
    float dist = rng_range(&w->rng, 0.0f, w->nest.radius * 0.8f);
    return vec2_add(w->nest.pos, vec2_scale(vec2_from_angle(angle), dist));
}

/** @brief Fill an ant slot with a freshly bred brain */
static void respawn_ant(World *w, int index) {
    Genome genome;
    evo_breed(&w->evo, &w->cfg, &genome, &w->rng);
    ant_init(&w->ants[index], w->next_ant_id++, nest_spawn_point(w), &genome, &w->rng);
}

/* ============== Lifetime ============== */

static bool alloc_systems(World *w) {
    if (!pheromone_init(&w->pheromones, w->width, w->height, PHEROMONE_CELL_SIZE)) return false;
    if (!walls_init(&w->walls, w->width, w->height)) return false;
    if (!shash_init(&w->ant_grid, w->width, w->height, ANT_GRID_CELL_SIZE, MAX_POPULATION)) {
        return false;
    }
    return true;
}

static void free_systems(World *w) {
    pheromone_free(&w->pheromones);
    walls_free(&w->walls);
    shash_free(&w->ant_grid);
}

World *world_create(const SimConfig *cfg, uint32_t seed) {
    World *w = calloc(1, sizeof(World));
    if (!w) return NULL;

    w->cfg = *cfg;
    sim_config_sanitize(&w->cfg);
    w->width = w->cfg.world_width;
    w->height = w->cfg.world_height;

    w->ants = calloc(MAX_POPULATION, sizeof(Ant));
    w->ant_positions = calloc(MAX_POPULATION, sizeof(Vec2));
    w->food = calloc(INITIAL_FOOD_CAPACITY, sizeof(FoodSource));
    w->food_capacity = INITIAL_FOOD_CAPACITY;

    if (!w->ants || !w->ant_positions || !w->food || !alloc_systems(w)) {
        world_destroy(w);
        return NULL;
    }

    world_reset(w, seed);
    return w;
}

void world_destroy(World *w) {
    if (!w) return;
    free_systems(w);
    free(w->ants);
    free(w->ant_positions);
    free(w->food);
    free(w->events);
    free(w);
}

void world_reset(World *w, uint32_t seed) {
    sim_config_sanitize(&w->cfg);
    rng_init(&w->rng, seed);
    w->seed = seed;

    /* Resize the fixed-size systems if the world dimensions changed */
    if (w->width != w->cfg.world_width || w->height != w->cfg.world_height) {
        free_systems(w);
        w->width = w->cfg.world_width;
        w->height = w->cfg.world_height;
        if (!alloc_systems(w)) return;
    } else {
        pheromone_clear(&w->pheromones);
    }

    w->tick = 0;
    w->event_count = 0;
    w->marker_count = 0;
    w->next_ant_id = 1;
    w->ant_count = 0;

    w->nest.pos = vec2((float)w->width * 0.5f, (float)w->height * 0.5f);
    w->nest.radius = NEST_RADIUS;
    w->nest.food_stored = 0.0f;

    evo_init(&w->evo);
    memset(&w->stats, 0, sizeof(w->stats));

    walls_generate_maze(&w->walls, w->nest.pos, NEST_KEEP_CLEAR, &w->rng);
    spawn_food_sources(w);
    world_apply_population(w);

    /* Give the first step a valid broadphase */
    for (int i = 0; i < w->ant_count; i++) w->ant_positions[i] = w->ants[i].pos;
    shash_build(&w->ant_grid, w->ant_positions, w->ant_count);
}

/* ============== Food ============== */

bool world_add_food(World *w, float x, float y, float amount) {
    if (x < 0.0f || y < 0.0f || x >= (float)w->width || y >= (float)w->height) return false;
    if (walls_overlaps_circle(&w->walls, x, y, FOOD_RADIUS)) return false;

    if (w->food_count >= w->food_capacity) {
        int new_cap = w->food_capacity * 2;
        FoodSource *grown = realloc(w->food, sizeof(FoodSource) * (size_t)new_cap);
        if (!grown) return false;
        w->food = grown;
        w->food_capacity = new_cap;
    }

    w->food[w->food_count++] = (FoodSource){vec2(x, y), amount, amount};
    return true;
}

float world_take_food(World *w, int food_index) {
    if (food_index < 0 || food_index >= w->food_count) return 0.0f;
    FoodSource *f = &w->food[food_index];
    if (f->amount <= 0.0f) return 0.0f;

    float taken = fminf(1.0f, f->amount);
    f->amount -= taken;
    return taken;
}

/** @brief Move depleted sources to a fresh spot with a fresh stock */
static void respawn_depleted_food(World *w) {
    for (int i = 0; i < w->food_count; i++) {
        if (w->food[i].amount > 0.0f) continue;
        Vec2 p;
        if (!random_free_position(w, FOOD_RADIUS * 2.0f, &p)) continue;
        w->food[i].pos = p;
        w->food[i].amount = rng_range(&w->rng, w->cfg.food_min, w->cfg.food_max);
        w->food[i].max_amount = w->food[i].amount;
    }
}

/* ============== Walls, population, lookup ============== */

void world_new_maze(World *w) {
    walls_generate_maze(&w->walls, w->nest.pos, NEST_KEEP_CLEAR, &w->rng);

    /* Drop food that the new walls swallowed */
    int write = 0;
    for (int i = 0; i < w->food_count; i++) {
        if (!walls_overlaps_circle(&w->walls, w->food[i].pos.x, w->food[i].pos.y, FOOD_RADIUS)) {
            w->food[write++] = w->food[i];
        }
    }
    w->food_count = write;

    /* Free any ant that is now embedded in a wall */
    for (int i = 0; i < w->ant_count; i++) {
        walls_resolve_circle(&w->walls, &w->ants[i].pos, ANT_RADIUS, NULL);
    }
}

void world_apply_population(World *w) {
    int target = clampi(w->cfg.population, 1, MAX_POPULATION);
    while (w->ant_count < target) {
        respawn_ant(w, w->ant_count);
        w->ant_count++;
    }
    if (w->ant_count > target) {
        w->ant_count = target;
    }
}

int world_ant_at(const World *w, float x, float y, float radius) {
    float best_sq = radius * radius;
    int best = -1;
    Vec2 p = vec2(x, y);
    for (int i = 0; i < w->ant_count; i++) {
        float d_sq = vec2_dist_sq(p, w->ants[i].pos);
        if (d_sq < best_sq) {
            best_sq = d_sq;
            best = i;
        }
    }
    return best;
}

int world_ant_by_id(const World *w, uint32_t id) {
    for (int i = 0; i < w->ant_count; i++) {
        if (w->ants[i].id == id) return i;
    }
    return -1;
}

/* ============== Statistics ============== */

const StatsSample *stats_sample(const WorldStats *s, int i) {
    if (i < 0 || i >= s->sample_count) return NULL;
    int oldest = (s->sample_head - s->sample_count + STATS_HISTORY_LEN) % STATS_HISTORY_LEN;
    return &s->samples[(oldest + i) % STATS_HISTORY_LEN];
}

const GenerationStats *stats_generation(const WorldStats *s, int i) {
    if (i < 0 || i >= s->gen_count) return NULL;
    int oldest = (s->gen_head - s->gen_count + GEN_HISTORY_LEN) % GEN_HISTORY_LEN;
    return &s->gens[(oldest + i) % GEN_HISTORY_LEN];
}

static void record_sample(World *w) {
    WorldStats *s = &w->stats;
    s->samples[s->sample_head] = (StatsSample){
        w->nest.food_stored, w->ant_count, s->carrying, s->avg_energy, s->window_deliveries
    };
    s->sample_head = (s->sample_head + 1) % STATS_HISTORY_LEN;
    if (s->sample_count < STATS_HISTORY_LEN) s->sample_count++;
    s->window_deliveries = 0;
}

static void record_generation(World *w) {
    WorldStats *s = &w->stats;
    const Evolution *evo = &w->evo;
    float avg = evo->gen_evaluated > 0 ? evo->gen_sum / (float)evo->gen_evaluated : 0.0f;

    s->gens[s->gen_head] = (GenerationStats){
        evo->generation, evo->gen_best, avg, evo->gen_evaluated, s->gen_deliveries
    };
    s->gen_head = (s->gen_head + 1) % GEN_HISTORY_LEN;
    if (s->gen_count < GEN_HISTORY_LEN) s->gen_count++;
    s->gen_deliveries = 0;
}

/* ============== Death markers ============== */

static void add_death_marker(World *w, Vec2 pos, DeathCause cause) {
    if (w->marker_count >= MAX_DEATH_MARKERS) {
        /* Drop the oldest */
        memmove(&w->markers[0], &w->markers[1], sizeof(DeathMarker) * (MAX_DEATH_MARKERS - 1));
        w->marker_count = MAX_DEATH_MARKERS - 1;
    }
    w->markers[w->marker_count++] = (DeathMarker){pos, DEATH_MARKER_DURATION, cause};
}

static void update_death_markers(World *w) {
    int write = 0;
    for (int i = 0; i < w->marker_count; i++) {
        if (--w->markers[i].ttl > 0) {
            w->markers[write++] = w->markers[i];
        }
    }
    w->marker_count = write;
}

/* ============== Generations ============== */

typedef struct {
    int index;
    float fitness;
} AntScore;

static int compare_score_asc(const void *pa, const void *pb) {
    float a = ((const AntScore *)pa)->fitness;
    float b = ((const AntScore *)pb)->fitness;
    return (a > b) - (a < b);
}

/**
 * @brief Score every living ant, promote the best genomes, and replace the worst ants
 *
 * Without this, ants that forage well keep topping up their energy and never
 * die, so their genomes would never be evaluated at all.
 */
static void finish_generation(World *w) {
    for (int i = 0; i < w->ant_count; i++) {
        evo_submit(&w->evo, &w->ants[i].genome, ant_fitness(&w->ants[i]));
    }

    record_generation(w);
    int generation = w->evo.generation;
    evo_finish_generation(&w->evo, w->cfg.elite_count);

    /* Replace the weakest ants with children of the new elites */
    int replace = (int)((float)w->ant_count * w->cfg.replace_fraction);
    if (replace > 0 && w->ant_count > 1) {
        AntScore *scores = malloc(sizeof(AntScore) * (size_t)w->ant_count);
        if (scores) {
            for (int i = 0; i < w->ant_count; i++) {
                scores[i] = (AntScore){i, ant_fitness(&w->ants[i])};
            }
            qsort(scores, (size_t)w->ant_count, sizeof(AntScore), compare_score_asc);
            if (replace > w->ant_count) replace = w->ant_count;
            for (int i = 0; i < replace; i++) {
                respawn_ant(w, scores[i].index);
            }
            free(scores);
        }
    }

    /* Fitness is measured per generation, so clear the per-generation counters */
    for (int i = 0; i < w->ant_count; i++) {
        w->ants[i].gen_age = 0;
        w->ants[i].gen_deliveries = 0;
    }

    emit(w, SIM_EVENT_GENERATION, 0, w->nest.pos, generation);
}

/* ============== Step ============== */

void world_step(World *w) {
    w->tick++;
    w->event_count = 0;

    pheromone_evaporate(&w->pheromones, w->cfg.trail_evaporation, w->cfg.danger_evaporation);

    /* Broadphase over this tick's starting positions */
    for (int i = 0; i < w->ant_count; i++) {
        w->ant_positions[i] = w->ants[i].pos;
    }
    shash_build(&w->ant_grid, w->ant_positions, w->ant_count);

    int carrying = 0;
    float energy_sum = 0.0f;

    for (int i = 0; i < w->ant_count; i++) {
        Ant *ant = &w->ants[i];
        AntOutcome outcome = ant_update(ant, i, w);

        switch (outcome) {
            case ANT_DELIVERED:
                w->nest.food_stored += 1.0f;
                w->stats.total_deliveries++;
                w->stats.window_deliveries++;
                w->stats.gen_deliveries++;
                emit(w, SIM_EVENT_DELIVERY, ant->id, ant->pos, 0);
                break;

            case ANT_DIED_STARVED:
            case ANT_DIED_STUCK: {
                DeathCause cause = (outcome == ANT_DIED_STARVED) ? DEATH_STARVED : DEATH_STUCK;
                if (cause == DEATH_STARVED) w->stats.deaths_starved++;
                else w->stats.deaths_stuck++;

                add_death_marker(w, ant->pos, cause);
                pheromone_deposit(&w->pheromones, PHEROMONE_DANGER,
                                  ant->pos.x, ant->pos.y, w->cfg.danger_on_death);
                evo_submit(&w->evo, &ant->genome, ant_fitness(ant));
                emit(w, SIM_EVENT_DEATH, ant->id, ant->pos, (int)cause);

                respawn_ant(w, i);
                break;
            }

            case ANT_OK:
            default:
                break;
        }

        if (w->ants[i].state == ANT_STATE_RETURNING) carrying++;
        energy_sum += w->ants[i].energy;
    }

    w->stats.carrying = carrying;
    w->stats.avg_energy = w->ant_count > 0 ? energy_sum / (float)w->ant_count : 0.0f;

    update_death_markers(w);

    if (w->tick % FOOD_RESPAWN_INTERVAL == 0) {
        respawn_depleted_food(w);
    }

    if (++w->evo.timer >= w->cfg.generation_ticks) {
        finish_generation(w);
    }

    if (w->tick % STATS_SAMPLE_INTERVAL == 0) {
        record_sample(w);
    }
}
