/**
 * @file vision.h
 * @brief Ray-based ant vision
 *
 * Each ant casts NN_NUM_VISION_RAYS rays across a VISION_FOV_DEGREES arc.
 * Every ray reports how close the nearest wall, ant and food source are,
 * as closeness in [0, 1]: 1 = touching, 0 = nothing within range.
 */

#ifndef VISION_H
#define VISION_H

#include "types.h"

struct World;

typedef struct {
    float wall;
    float ant;
    float food;
} VisionRay;

/** @brief Angle of ray i relative to the ant's heading */
float vision_ray_angle(int i);

/**
 * @brief Cast all rays for one ant
 * @param self_index Index of the ant in world->ants, excluded from ant hits
 */
void vision_cast(const struct World *w, int self_index, Vec2 pos, float heading,
                 VisionRay out[NN_NUM_VISION_RAYS]);

/** @brief Write rays as network inputs: [wall x7, ant x7, food x7] */
void vision_to_inputs(const VisionRay rays[NN_NUM_VISION_RAYS], float *inputs);

#endif /* VISION_H */
