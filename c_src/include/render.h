/**
 * @file render.h
 * @brief SDL2 rendering functions
 */

#ifndef RENDER_H
#define RENDER_H

#include <SDL.h>
#include "types.h"

/**
 * @brief Initialize rendering system
 * @param width Screen width
 * @param height Screen height
 * @return SDL renderer, or NULL on failure
 */
SDL_Renderer* render_init(int *width, int *height);

/**
 * @brief Cleanup rendering system
 */
void render_cleanup(SDL_Renderer *renderer);

/**
 * @brief Clear screen with background color
 */
void render_clear(SDL_Renderer *renderer);

/**
 * @brief Present rendered frame
 */
void render_present(SDL_Renderer *renderer);

/**
 * @brief Draw a filled circle
 */
void render_circle(SDL_Renderer *renderer, int x, int y, int radius, Color color);

/**
 * @brief Draw a filled rectangle
 */
void render_rect(SDL_Renderer *renderer, int x, int y, int w, int h, Color color);

/**
 * @brief Draw the colony
 */
void render_colony(SDL_Renderer *renderer, const Colony *colony);

/**
 * @brief Draw all ants
 */
void render_ants(SDL_Renderer *renderer, const Colony *colony);

/**
 * @brief Draw all food sources
 */
void render_food_sources(SDL_Renderer *renderer, const Colony *colony);

/**
 * @brief Draw all walls
 */
void render_walls(SDL_Renderer *renderer, const Colony *colony);

/**
 * @brief Draw death markers
 */
void render_death_markers(SDL_Renderer *renderer, const Colony *colony);

/**
 * @brief Draw pheromone overlay
 */
void render_pheromones(SDL_Renderer *renderer, const Colony *colony);

/**
 * @brief Draw grid overlay
 */
void render_grid(SDL_Renderer *renderer, int width, int height);

/**
 * @brief Draw HUD with stats
 */
void render_hud(SDL_Renderer *renderer, const SimState *state);

/**
 * @brief Draw keybind hints
 */
void render_hints(SDL_Renderer *renderer);

#endif /* RENDER_H */
