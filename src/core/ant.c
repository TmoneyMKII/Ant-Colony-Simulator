/**
 * @file ant.c
 * @brief Ant behaviour: sense, steer, move, interact
 */

#include "ant.h"
#include "world.h"
#include "utils.h"
#include <string.h>

/* ============== Setup ============== */

void ant_init(Ant *ant, uint32_t id, Vec2 pos, const Genome *genome, Rng *rng) {
    memset(ant, 0, sizeof(*ant));
    ant->id = id;
    ant->pos = pos;
    ant->checkpoint = pos;
    ant->heading = rng_range(rng, 0.0f, TWO_PI_F);
    ant->state = ANT_STATE_FORAGING;
    ant->energy = ANT_MAX_ENERGY;
    ant->genome = *genome;
}

float ant_fitness(const Ant *ant) {
    return (float)ant->gen_deliveries * FITNESS_PER_DELIVERY +
           (float)ant->gen_age * FITNESS_PER_TICK;
}

/* ============== Steering ============== */

/** @brief Nearest food source with stock, within range; -1 if none */
static int nearest_food(const World *w, Vec2 pos, float range) {
    float best_sq = range * range;
    int best = -1;
    for (int i = 0; i < w->food_count; i++) {
        if (w->food[i].amount <= 0.0f) continue;
        float d_sq = vec2_dist_sq(pos, w->food[i].pos);
        if (d_sq < best_sq) {
            best_sq = d_sq;
            best = i;
        }
    }
    return best;
}

/** @brief Hand-written trail following, used when cfg.brain_mode is BRAIN_CLASSIC */
static void steer_classic(Ant *ant, World *w) {
    const SimConfig *cfg = &w->cfg;
    ant->speed = cfg->ant_speed;

    float target = 0.0f;
    bool have_target = false;

    if (ant->state == ANT_STATE_FORAGING) {
        int food = nearest_food(w, ant->pos, ANT_SMELL_RANGE);
        if (food >= 0) {
            target = vec2_angle_to(ant->pos, w->food[food].pos);
            have_target = true;
        } else {
            have_target = pheromone_steer(&w->pheromones, PHEROMONE_FOOD,
                                          ant->pos.x, ant->pos.y, ant->heading, &target);
        }
    } else {
        have_target = pheromone_steer(&w->pheromones, PHEROMONE_HOME,
                                      ant->pos.x, ant->pos.y, ant->heading, &target);
        if (!have_target) {
            /* No trail to follow: head home directly (dead reckoning) */
            target = vec2_angle_to(ant->pos, w->nest.pos);
            have_target = true;
        }
    }

    if (have_target) {
        ant->heading = lerp_angle(ant->heading, target, cfg->trail_sensitivity);
    } else {
        ant->heading = normalize_angle(ant->heading +
                                       rng_range(&w->rng, -cfg->turn_rate, cfg->turn_rate) * 0.5f);
    }
}

/** @brief Evolved network steering */
static void steer_neural(Ant *ant, int self_index, World *w) {
    const SimConfig *cfg = &w->cfg;

    vision_cast(w, self_index, ant->pos, ant->heading, ant->vision);

    float inputs[NN_INPUT_SIZE];
    vision_to_inputs(ant->vision, inputs);

    int k = NN_VISION_INPUTS;
    Vec2 ahead = vec2_add(ant->pos, vec2_scale(vec2_from_angle(ant->heading), 20.0f));
    inputs[k++] = clampf(pheromone_sample(&w->pheromones, PHEROMONE_FOOD, ahead.x, ahead.y) /
                         PHEROMONE_MAX_VALUE, 0.0f, 1.0f);
    inputs[k++] = clampf(pheromone_sample(&w->pheromones, PHEROMONE_HOME, ahead.x, ahead.y) /
                         PHEROMONE_MAX_VALUE, 0.0f, 1.0f);

    float nest_dist = vec2_dist(ant->pos, w->nest.pos);
    float max_dist = sqrtf((float)(w->width * w->width + w->height * w->height));
    inputs[k++] = clampf(nest_dist / max_dist, 0.0f, 1.0f);
    inputs[k++] = normalize_angle(vec2_angle_to(ant->pos, w->nest.pos) - ant->heading) / PI_F;
    inputs[k++] = (ant->state == ANT_STATE_RETURNING) ? 1.0f : 0.0f;
    inputs[k++] = ant->energy / ANT_MAX_ENERGY;

    nn_forward(&ant->genome, inputs, &ant->nn);
    const float *out = ant->nn.outputs;

    ant->heading = normalize_angle(ant->heading + out[0] * cfg->turn_rate);
    ant->speed = cfg->ant_speed * clampf(0.5f + (out[1] + 1.0f) * 0.5f, 0.5f, 1.5f);
    if (out[2] > 0.5f) {
        ant->heading = normalize_angle(ant->heading + rng_range(&w->rng, -0.2f, 0.2f));
    }
}

/* ============== Movement ============== */

static void move(Ant *ant, World *w) {
    ant->pos = vec2_add(ant->pos, vec2_scale(vec2_from_angle(ant->heading), ant->speed));

    /* Walls: push out, then bounce the heading off the contact normal */
    Vec2 normal;
    if (walls_resolve_circle(&w->walls, &ant->pos, ANT_RADIUS, &normal)) {
        Vec2 dir = vec2_from_angle(ant->heading);
        float into = vec2_dot(dir, normal);
        if (into < 0.0f) {
            dir = vec2_sub(dir, vec2_scale(normal, 2.0f * into));
            ant->heading = atan2f(dir.y, dir.x);
        } else {
            ant->heading = atan2f(normal.y, normal.x);
        }
        ant->heading = normalize_angle(ant->heading + rng_range(&w->rng, -0.3f, 0.3f));
    }

    /* World edges: clamp and reflect */
    const float margin = ANT_RADIUS + 2.0f;
    float right = (float)w->width - margin;
    float bottom = (float)w->height - margin;

    if (ant->pos.x < margin) {
        ant->pos.x = margin;
        ant->heading = normalize_angle(PI_F - ant->heading);
    } else if (ant->pos.x > right) {
        ant->pos.x = right;
        ant->heading = normalize_angle(PI_F - ant->heading);
    }
    if (ant->pos.y < margin) {
        ant->pos.y = margin;
        ant->heading = normalize_angle(-ant->heading);
    } else if (ant->pos.y > bottom) {
        ant->pos.y = bottom;
        ant->heading = normalize_angle(-ant->heading);
    }
}

/** @brief True if the ant has failed too many progress checks */
static bool check_stuck(Ant *ant, Rng *rng) {
    if (++ant->stuck_timer < STUCK_CHECK_INTERVAL) return false;

    ant->stuck_timer = 0;
    float moved_sq = vec2_dist_sq(ant->pos, ant->checkpoint);
    ant->checkpoint = ant->pos;

    if (moved_sq >= STUCK_MIN_MOVEMENT * STUCK_MIN_MOVEMENT) {
        ant->stuck_strikes = 0;
        return false;
    }

    if (++ant->stuck_strikes >= STUCK_MAX_STRIKES) return true;

    ant->heading = rng_range(rng, 0.0f, TWO_PI_F);  /* try a new direction */
    return false;
}

/* ============== Interactions ============== */

static void try_pickup(Ant *ant, World *w) {
    int idx = nearest_food(w, ant->pos, FOOD_PICKUP_RANGE);
    if (idx < 0) return;
    if (world_take_food(w, idx) <= 0.0f) return;

    ant->state = ANT_STATE_RETURNING;
    ant->knows_food = true;
    ant->last_food = w->food[idx].pos;
    ant->heading = vec2_angle_to(ant->pos, w->nest.pos);
}

static bool try_dropoff(Ant *ant, World *w) {
    if (vec2_dist_sq(ant->pos, w->nest.pos) >= NEST_DROPOFF_RANGE * NEST_DROPOFF_RANGE) {
        return false;
    }

    ant->state = ANT_STATE_FORAGING;
    ant->deliveries++;
    ant->gen_deliveries++;
    ant->energy = fminf(ANT_MAX_ENERGY, ant->energy + w->cfg.delivery_energy);

    /* Head back toward the last food found, or pick a new direction */
    ant->heading = ant->knows_food ? vec2_angle_to(ant->pos, ant->last_food)
                                   : rng_range(&w->rng, 0.0f, TWO_PI_F);
    return true;
}

static void deposit_trail(Ant *ant, World *w) {
    if (--ant->deposit_timer > 0) return;
    ant->deposit_timer = w->cfg.deposit_interval;

    /* Returning ants mark the way to food; foragers mark the way home */
    PheromoneType type = (ant->state == ANT_STATE_RETURNING) ? PHEROMONE_FOOD : PHEROMONE_HOME;
    pheromone_deposit(&w->pheromones, type, ant->pos.x, ant->pos.y, w->cfg.deposit_amount);
}

/* ============== Update ============== */

AntOutcome ant_update(Ant *ant, int self_index, World *w) {
    ant->age++;
    ant->gen_age++;

    ant->energy -= w->cfg.energy_drain;
    if (ant->energy <= 0.0f) return ANT_DIED_STARVED;

    if (w->cfg.brain_mode == BRAIN_NEURAL) {
        steer_neural(ant, self_index, w);
    } else {
        steer_classic(ant, w);
    }

    move(ant, w);

    if (check_stuck(ant, &w->rng)) return ANT_DIED_STUCK;

    AntOutcome outcome = ANT_OK;
    if (ant->state == ANT_STATE_FORAGING) {
        try_pickup(ant, w);
    } else if (try_dropoff(ant, w)) {
        outcome = ANT_DELIVERED;
    }

    deposit_trail(ant, w);
    return outcome;
}
