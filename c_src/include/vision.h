/**
 * @file vision.h
 * @brief Ray-based vision system for ants
 */

#ifndef VISION_H
#define VISION_H

#include "types.h"

/**
 * @brief Initialize vision system for an ant
 * Pre-computes ray angles based on FOV configuration
 */
void vision_init(AntVision *vision);

/**
 * @brief Cast all vision rays and update ray results
 * @param vision Vision system to update
 * @param ant_x Ant's X position
 * @param ant_y Ant's Y position
 * @param ant_direction Ant's heading in radians
 * @param wall_manager Wall collision system
 * @param ants Array of all ants
 * @param ant_count Number of ants
 * @param food_sources Array of food sources
 * @param food_count Number of food sources
 * @param exclude_id This ant's ID (exclude from detection)
 */
void vision_cast_rays(AntVision *vision,
                      float ant_x, float ant_y, float ant_direction,
                      const WallManager *wall_manager,
                      const Ant *ants, int ant_count,
                      const FoodSource *food_sources, int food_count,
                      uint32_t exclude_id);

/**
 * @brief Get vision inputs for neural network
 * @param vision Vision system
 * @param inputs Output array (must have NN_VISION_INPUTS = 21 elements)
 * 
 * Layout: [wall×7, ant×7, food×7] normalized 0-1 (closer = higher)
 */
void vision_get_inputs(const AntVision *vision, float *inputs);

/**
 * @brief Reset all rays to default state (nothing detected)
 */
void vision_reset(AntVision *vision);

#endif /* VISION_H */
