/**
 * @file render.h
 * @brief Drawing the world: camera, sprite batching, pheromone overlay
 *
 * The renderer only reads the World. All of its state (textures, batches,
 * camera) lives here so the simulation stays free of SDL.
 */

#ifndef RENDER_H
#define RENDER_H

#include <SDL.h>
#include "world.h"

/* ============== Camera ============== */

/**
 * @brief Maps world coordinates to a viewport rectangle in the window
 */
typedef struct {
    Vec2 center;        /**< World point shown at the centre of the viewport */
    float zoom;         /**< Screen pixels per world unit */
    float min_zoom;
    float max_zoom;
    SDL_Rect viewport;  /**< Where the world is drawn, in window pixels */
} Camera;

void camera_init(Camera *cam, SDL_Rect viewport, int world_width, int world_height);

/** @brief Frame the whole world in the viewport */
void camera_fit(Camera *cam, int world_width, int world_height);

Vec2 camera_to_screen(const Camera *cam, Vec2 world);
Vec2 camera_to_world(const Camera *cam, Vec2 screen);

/** @brief Drag the view by a screen-space delta */
void camera_pan(Camera *cam, float dx, float dy);

/** @brief Zoom by a factor, keeping the world point under `anchor` fixed */
void camera_zoom_at(Camera *cam, Vec2 anchor_screen, float factor);

/** @brief Keep the view over the world after a resize or world change */
void camera_clamp(Camera *cam, int world_width, int world_height);

/* ============== What to draw ============== */

typedef struct {
    bool grid;
    bool walls;
    bool food;
    bool nest;
    bool ants;
    bool markers;
    bool trail_food;
    bool trail_home;
    bool trail_danger;
    bool vision_rays;   /**< For the selected ant */
    bool ant_headings;
} RenderOptions;

/** @brief Sensible defaults: everything except trails and debug overlays */
void render_options_defaults(RenderOptions *opt);

/* ============== Renderer ============== */

typedef struct WorldRenderer WorldRenderer;

WorldRenderer *world_renderer_create(SDL_Renderer *renderer);
void world_renderer_destroy(WorldRenderer *wr);

/**
 * @brief Draw the world
 * @param selected Index of the highlighted ant, or -1
 */
void world_renderer_draw(WorldRenderer *wr, SDL_Renderer *renderer, const World *w,
                         const Camera *cam, const RenderOptions *opt, int selected);

/** @brief Ants drawn by the last call (useful as a rendering-cost readout) */
int world_renderer_last_ant_count(const WorldRenderer *wr);

#endif /* RENDER_H */
