/**
 * @file walls.h
 * @brief Wall/obstacle system with spatial partitioning
 */

#ifndef WALLS_H
#define WALLS_H

#include "types.h"

/**
 * @brief Create a new wall manager
 * @param width World width in pixels
 * @param height World height in pixels
 * @param offset_x X offset of simulation area
 * @param offset_y Y offset of simulation area
 * @return Pointer to new manager, or NULL on failure
 */
WallManager* walls_create(int width, int height, int offset_x, int offset_y);

/**
 * @brief Free wall manager memory
 */
void walls_destroy(WallManager *wm);

/**
 * @brief Clear all walls
 */
void walls_clear(WallManager *wm);

/**
 * @brief Generate a random maze
 */
void walls_generate_maze(WallManager *wm, float colony_x, float colony_y, float colony_radius);

/**
 * @brief Add a wall segment
 * @return true if added successfully
 */
bool walls_add(WallManager *wm, int x, int y, int width, int height);

/**
 * @brief Check if a point collides with any wall
 * @param wm Wall manager
 * @param x X position
 * @param y Y position
 * @param margin Extra margin around point
 * @param out_wall_idx Output: index of colliding wall (can be NULL)
 * @return true if collision detected
 */
bool walls_is_colliding(const WallManager *wm, float x, float y, int margin, int *out_wall_idx);

/**
 * @brief Get push-out vector for collision resolution
 * @param wm Wall manager
 * @param x X position
 * @param y Y position
 * @param margin Collision margin
 * @param out_dx Output: push X direction
 * @param out_dy Output: push Y direction
 * @return true if collision found
 */
bool walls_get_push_vector(const WallManager *wm, float x, float y, int margin,
                            float *out_dx, float *out_dy);

/**
 * @brief Get number of walls
 */
int walls_get_count(const WallManager *wm);

/**
 * @brief Get wall bounds by index
 */
const Rect* walls_get(const WallManager *wm, int index);

#endif /* WALLS_H */
