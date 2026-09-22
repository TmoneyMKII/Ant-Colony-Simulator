/**
 * @file app.c
 * @brief Window, main loop, input dispatch and camera control
 */

#include "app.h"
#include "ui.h"
#include "ui_panels.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WINDOW_TITLE     "Ant Colony Simulator"
#define DEFAULT_WIDTH    1600
#define DEFAULT_HEIGHT   900
#define BASE_FONT_SIZE   15.0f
#define SIM_BUDGET_MS    12.0   /* cap simulation work per frame so the UI stays live */
#define METRIC_PERIOD_MS 500

const float SPEED_LEVELS[SPEED_LEVEL_COUNT] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};

/* ============== Setup ============== */

static Font *load_ui_font(SDL_Renderer *renderer, float size) {
    static const char *paths[] = {
#ifdef ANTSIM_ASSET_DIR
        ANTSIM_ASSET_DIR "/fonts/JetBrainsMono-Regular.ttf",
#endif
        "assets/fonts/JetBrainsMono-Regular.ttf",
        "../assets/fonts/JetBrainsMono-Regular.ttf",
    };

    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        Font *font = font_load(renderer, paths[i], size);
        if (font) return font;
    }
    return NULL;
}

static SDL_Rect viewport_for(const App *app) {
    return (SDL_Rect){0, 0, app->window_width, app->window_height};
}

bool app_init(App *app) {
    memset(app, 0, sizeof(*app));

    /* Windows stretches DPI-unaware windows, which would push part of the
       UI off screen and blur everything. Ask for real pixels instead. */
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    /* Scale the interface with the display, and keep the window inside the
       desktop work area so it never opens partly offscreen. */
    float dpi_scale = 1.0f;
    float ddpi = 0.0f, hdpi = 0.0f, vdpi = 0.0f;
    if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0 && ddpi > 0.0f) {
        dpi_scale = clampf(ddpi / 96.0f, 1.0f, 2.5f);
    }

    int width = (int)(DEFAULT_WIDTH * dpi_scale);
    int height = (int)(DEFAULT_HEIGHT * dpi_scale);
    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
        if (width > usable.w) width = usable.w;
        if (height > usable.h) height = usable.h;
    }

    /* No ALLOW_HIGHDPI: it makes renderer pixels differ from the window
       coordinates that mouse events use, which offsets every click. */
    app->window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   width, height, SDL_WINDOW_RESIZABLE);
    if (!app->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1,
                                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        /* Software rendering is slow but better than refusing to start */
        app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!app->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);

    SDL_GetRendererOutputSize(app->renderer, &app->window_width, &app->window_height);
    app->ui_scale = dpi_scale;

    app->font = load_ui_font(app->renderer, BASE_FONT_SIZE * app->ui_scale);
    if (!app->font) {
        fprintf(stderr, "Could not load assets/fonts/JetBrainsMono-Regular.ttf\n");
        return false;
    }
    if (!ui_init(app->font, app->ui_scale)) {
        fprintf(stderr, "UI init failed\n");
        return false;
    }

    sim_config_defaults(&app->cfg);
    app->seed = (uint32_t)time(NULL);
    snprintf(app->seed_text, sizeof(app->seed_text), "%u", app->seed);

    app->world = world_create(&app->cfg, app->seed);
    if (!app->world) {
        fprintf(stderr, "World creation failed\n");
        return false;
    }

    app->world_renderer = world_renderer_create(app->renderer);
    if (!app->world_renderer) {
        fprintf(stderr, "Renderer resources failed\n");
        return false;
    }

    render_options_defaults(&app->view);
    camera_init(&app->camera, viewport_for(app), app->world->width, app->world->height);

    app->running = true;
    app->paused = false;
    app->show_panels = true;
    app->speed_index = SPEED_LEVEL_NORMAL;
    app->tool = TOOL_SELECT;

    return true;
}

void app_shutdown(App *app) {
    ui_shutdown();
    if (app->world_renderer) world_renderer_destroy(app->world_renderer);
    if (app->world) world_destroy(app->world);
    if (app->font) font_destroy(app->font);
    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);
    SDL_Quit();
}

/* ============== Actions ============== */

float app_sim_speed(const App *app) {
    return SPEED_LEVELS[clampi(app->speed_index, 0, SPEED_LEVEL_COUNT - 1)];
}

void app_set_speed_index(App *app, int index) {
    app->speed_index = clampi(index, 0, SPEED_LEVEL_COUNT - 1);
    app->max_speed = false;
}

int app_selected_index(const App *app) {
    if (app->selected_id == 0) return -1;
    return world_ant_by_id(app->world, app->selected_id);
}

void app_reset_world(App *app, bool new_seed) {
    if (new_seed) {
        app->seed = (uint32_t)SDL_GetTicks() ^ (uint32_t)(uintptr_t)app;
        snprintf(app->seed_text, sizeof(app->seed_text), "%u", app->seed);
    }

    sim_config_sanitize(&app->cfg);
    app->world->cfg = app->cfg;
    world_reset(app->world, app->seed);

    app->selected_id = 0;
    app->follow_selected = false;
    app->tick_accumulator = 0.0f;
    camera_init(&app->camera, viewport_for(app), app->world->width, app->world->height);
}

static void toggle_fullscreen(App *app) {
    app->fullscreen = !app->fullscreen;
    SDL_SetWindowFullscreen(app->window, app->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void app_do_action(App *app, Action action) {
    switch (action) {
        case ACTION_QUIT:
            app->running = false;
            break;

        case ACTION_TOGGLE_PAUSE:
            app->paused = !app->paused;
            break;

        case ACTION_STEP_ONCE:
            app->paused = true;
            app->step_once = true;
            break;

        case ACTION_SPEED_UP:
            app_set_speed_index(app, app->speed_index + 1);
            break;

        case ACTION_SPEED_DOWN:
            app_set_speed_index(app, app->speed_index - 1);
            break;

        case ACTION_SPEED_MAX:
            app->max_speed = !app->max_speed;
            break;

        case ACTION_RESET:
            app_reset_world(app, false);
            break;

        case ACTION_RESET_NEW_SEED:
            app_reset_world(app, true);
            break;

        case ACTION_NEW_MAZE:
            world_new_maze(app->world);
            break;

        case ACTION_TOGGLE_TRAILS: {
            bool on = !(app->view.trail_food || app->view.trail_home);
            app->view.trail_food = on;
            app->view.trail_home = on;
            break;
        }

        case ACTION_TOGGLE_DANGER:
            app->view.trail_danger = !app->view.trail_danger;
            break;

        case ACTION_TOGGLE_GRID:
            app->view.grid = !app->view.grid;
            break;

        case ACTION_TOGGLE_MARKERS:
            app->view.markers = !app->view.markers;
            break;

        case ACTION_TOGGLE_VISION:
            app->view.vision_rays = !app->view.vision_rays;
            break;

        case ACTION_TOGGLE_BRAIN:
            app->cfg.brain_mode = (app->cfg.brain_mode == BRAIN_NEURAL) ? BRAIN_CLASSIC
                                                                        : BRAIN_NEURAL;
            break;

        case ACTION_TOGGLE_PANELS:
            app->show_panels = !app->show_panels;
            break;

        case ACTION_TOGGLE_FULLSCREEN:
            toggle_fullscreen(app);
            break;

        case ACTION_FIT_CAMERA:
            camera_fit(&app->camera, app->world->width, app->world->height);
            break;

        case ACTION_FOLLOW_SELECTED:
            app->follow_selected = !app->follow_selected;
            break;

        case ACTION_DESELECT:
            /* Esc clears the selection first, and quits when there is none */
            if (app->selected_id != 0) {
                app->selected_id = 0;
                app->follow_selected = false;
            } else {
                app->running = false;
            }
            break;

        case ACTION_TOOL_SELECT:
            app->tool = TOOL_SELECT;
            break;

        case ACTION_TOOL_FOOD:
            app->tool = TOOL_FOOD;
            break;

        case ACTION_NONE:
        case ACTION_COUNT:
        default:
            break;
    }
}

/* ============== Mouse in the world ============== */

static void select_ant_at(App *app, int screen_x, int screen_y) {
    Vec2 world = camera_to_world(&app->camera, vec2((float)screen_x, (float)screen_y));
    float radius = fmaxf(ANT_RADIUS * 2.0f, 14.0f / app->camera.zoom);

    int index = world_ant_at(app->world, world.x, world.y, radius);
    app->selected_id = (index >= 0) ? app->world->ants[index].id : 0;
    if (index < 0) app->follow_selected = false;
}

static void place_food_at(App *app, int screen_x, int screen_y) {
    Vec2 world = camera_to_world(&app->camera, vec2((float)screen_x, (float)screen_y));
    world_add_food(app->world, world.x, world.y, 50.0f);
}

static void handle_world_click(App *app, const SDL_MouseButtonEvent *button) {
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    bool food_modifier = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

    if (app->tool == TOOL_FOOD || food_modifier) {
        place_food_at(app, button->x, button->y);
    } else {
        select_ant_at(app, button->x, button->y);
    }
}

/* ============== Events ============== */

static void on_window_resized(App *app) {
    SDL_GetRendererOutputSize(app->renderer, &app->window_width, &app->window_height);
    app->camera.viewport = viewport_for(app);
    camera_clamp(&app->camera, app->world->width, app->world->height);
}

static void handle_events(App *app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ui_handle_event(&event);

        switch (event.type) {
            case SDL_QUIT:
                app->running = false;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    on_window_resized(app);
                }
                break;

            case SDL_KEYDOWN:
                if (!ui_wants_keyboard() && event.key.repeat == 0) {
                    app_do_action(app, input_action_for_key(event.key.keysym.sym));
                }
                break;

            case SDL_MOUSEWHEEL:
                if (!ui_wants_mouse() && event.wheel.y != 0) {
                    int mx, my;
                    SDL_GetMouseState(&mx, &my);
                    float factor = event.wheel.y > 0 ? 1.15f : 1.0f / 1.15f;
                    camera_zoom_at(&app->camera, vec2((float)mx, (float)my), factor);
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (ui_wants_mouse()) break;
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handle_world_click(app, &event.button);
                } else if (event.button.button == SDL_BUTTON_RIGHT ||
                           event.button.button == SDL_BUTTON_MIDDLE) {
                    app->panning = true;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_RIGHT ||
                    event.button.button == SDL_BUTTON_MIDDLE) {
                    app->panning = false;
                }
                break;

            case SDL_MOUSEMOTION:
                if (app->panning) {
                    camera_pan(&app->camera, (float)event.motion.xrel, (float)event.motion.yrel);
                    camera_clamp(&app->camera, app->world->width, app->world->height);
                    app->follow_selected = false;
                }
                break;

            default:
                break;
        }
    }
}

/* ============== Simulation ============== */

/** @brief Push live parameter edits into the world */
static void sync_config(App *app) {
    sim_config_sanitize(&app->cfg);

    int previous_population = app->world->cfg.population;
    app->world->cfg = app->cfg;
    if (app->cfg.population != previous_population) {
        world_apply_population(app->world);
    }
}

/**
 * @brief Advance the simulation for this frame
 *
 * The simulation runs at a fixed SIM_TICK_RATE independent of the frame
 * rate; speed multiplies how many ticks a second of wall clock buys. Work
 * is capped per frame so a heavy setting slows the simulation rather than
 * freezing the window.
 */
static void advance_simulation(App *app, float dt) {
    app->steps_last_frame = 0;

    if (app->step_once) {
        world_step(app->world);
        app->steps_last_frame = 1;
        app->step_once = false;
        return;
    }
    if (app->paused) {
        app->tick_accumulator = 0.0f;
        return;
    }

    const uint64_t freq = SDL_GetPerformanceFrequency();
    const uint64_t start = SDL_GetPerformanceCounter();
    const double budget = SIM_BUDGET_MS / 1000.0;

    if (app->max_speed) {
        do {
            world_step(app->world);
            app->steps_last_frame++;
        } while ((double)(SDL_GetPerformanceCounter() - start) / (double)freq < budget);
        return;
    }

    app->tick_accumulator += dt * (float)SIM_TICK_RATE * app_sim_speed(app);
    while (app->tick_accumulator >= 1.0f) {
        world_step(app->world);
        app->steps_last_frame++;
        app->tick_accumulator -= 1.0f;

        if ((double)(SDL_GetPerformanceCounter() - start) / (double)freq >= budget) {
            app->tick_accumulator = 0.0f;   /* drop the backlog instead of stuttering */
            break;
        }
    }
}

static void update_metrics(App *app) {
    app->metric_frames++;
    app->metric_ticks += (uint64_t)app->steps_last_frame;

    uint32_t now = SDL_GetTicks();
    if (app->metric_time_ms == 0) {
        app->metric_time_ms = now;
        return;
    }
    uint32_t elapsed = now - app->metric_time_ms;
    if (elapsed >= METRIC_PERIOD_MS) {
        app->fps = (float)app->metric_frames * 1000.0f / (float)elapsed;
        app->sim_tps = (float)app->metric_ticks * 1000.0f / (float)elapsed;
        app->metric_frames = 0;
        app->metric_ticks = 0;
        app->metric_time_ms = now;
    }
}

/* ============== Main loop ============== */

void app_run(App *app) {
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t last = SDL_GetPerformanceCounter();

    while (app->running) {
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)((double)(now - last) / (double)freq);
        last = now;
        if (dt > 0.1f) dt = 0.1f;   /* after a stall, carry on rather than catching up */

        handle_events(app);
        sync_config(app);
        advance_simulation(app, dt);

        /* Follow mode keeps the camera on the inspected ant */
        int selected = app_selected_index(app);
        if (app->follow_selected && selected >= 0) {
            app->camera.center = app->world->ants[selected].pos;
        }

        ui_begin_frame();
        if (app->show_panels) {
            ui_panels_build(app);
        }
        ui_end_frame();

        world_renderer_draw(app->world_renderer, app->renderer, app->world,
                            &app->camera, &app->view, selected);
        ui_render(app->renderer);
        SDL_RenderPresent(app->renderer);

        update_metrics(app);
    }
}
