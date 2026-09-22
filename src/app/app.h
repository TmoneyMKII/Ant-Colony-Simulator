/**
 * @file app.h
 * @brief Application state: window, world, camera, view options, metrics
 */

#ifndef APP_H
#define APP_H

#include <SDL.h>
#include "world.h"
#include "render.h"
#include "font.h"
#include "input.h"

typedef enum {
    TOOL_SELECT = 0,
    TOOL_FOOD
} Tool;

#define SPEED_LEVEL_COUNT 6
extern const float SPEED_LEVELS[SPEED_LEVEL_COUNT];
#define SPEED_LEVEL_NORMAL 2    /* index of 1x */

typedef struct App {
    SDL_Window *window;
    SDL_Renderer *renderer;
    Font *font;
    WorldRenderer *world_renderer;
    World *world;

    /**
     * Parameters as edited by the UI. Live fields are copied into the world
     * every frame; world size and food setup only take effect on reset.
     */
    SimConfig cfg;

    Camera camera;
    RenderOptions view;

    bool running;
    bool paused;
    bool step_once;
    bool show_panels;
    bool fullscreen;

    int speed_index;
    bool max_speed;         /**< Run as many ticks per frame as fit the budget */
    float tick_accumulator;

    uint32_t seed;
    char seed_text[16];     /**< Backing store for the seed text field */

    Tool tool;
    uint32_t selected_id;   /**< 0 when no ant is selected */
    bool follow_selected;
    bool panning;

    float ui_scale;
    int window_width;
    int window_height;

    /* Metrics, refreshed about twice a second */
    float fps;
    float sim_tps;
    int steps_last_frame;
    uint64_t metric_ticks;
    int metric_frames;
    uint32_t metric_time_ms;
} App;

bool app_init(App *app);
void app_run(App *app);
void app_shutdown(App *app);

/** @brief Run one named action (from a key press or a UI button) */
void app_do_action(App *app, Action action);

/** @brief Restart the world; with new_seed, roll a fresh seed first */
void app_reset_world(App *app, bool new_seed);

/** @brief Ticks of simulation per second of wall clock at the current speed */
float app_sim_speed(const App *app);

/** @brief Index of the selected ant, or -1 if none or it died */
int app_selected_index(const App *app);

void app_set_speed_index(App *app, int index);

#endif /* APP_H */
