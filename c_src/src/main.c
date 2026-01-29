/**
 * @file main.c
 * @brief Ant Colony Simulator - Entry point and game loop
 * 
 * High-performance C implementation with SDL2 rendering.
 * 
 * Controls:
 *   SPACE   - Pause/Resume
 *   P       - Toggle pheromone visualization
 *   G       - Toggle grid
 *   R       - Reset colony
 *   M       - Generate new maze
 *   H       - Show hints
 *   ,/.     - Speed down/up
 *   F+Click - Add food
 *   ESC     - Quit
 */

#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "config.h"
#include "types.h"
#include "utils.h"
#include "colony.h"
#include "render.h"
#include "walls.h"
#include "pheromone.h"

/* ============== Global State ============== */

static SimState g_state = {0};

/* ============== Event Handling ============== */

static void handle_keydown(SDL_Keycode key) {
    switch (key) {
        case SDLK_ESCAPE:
            g_state.running = false;
            break;
            
        case SDLK_SPACE:
            g_state.paused = !g_state.paused;
            printf("Simulation %s\n", g_state.paused ? "PAUSED" : "RESUMED");
            break;
            
        case SDLK_p:
            g_state.show_pheromones = !g_state.show_pheromones;
            printf("Pheromones: %s\n", g_state.show_pheromones ? "ON" : "OFF");
            break;
            
        case SDLK_g:
            g_state.show_grid = !g_state.show_grid;
            printf("Grid: %s\n", g_state.show_grid ? "ON" : "OFF");
            break;
            
        case SDLK_n:
            g_state.show_neural_ui = !g_state.show_neural_ui;
            printf("Neural UI: %s\n", g_state.show_neural_ui ? "ON" : "OFF");
            break;
            
        case SDLK_h:
            g_state.show_hints = !g_state.show_hints;
            break;
            
        case SDLK_d:
            g_state.show_debug = !g_state.show_debug;
            printf("Debug: %s\n", g_state.show_debug ? "ON" : "OFF");
            break;
            
        case SDLK_r:
            printf("Resetting colony...\n");
            colony_reset(g_state.colony);
            break;
            
        case SDLK_m:
            printf("Generating new maze...\n");
            walls_generate_maze(g_state.colony->wall_manager, 
                               g_state.colony->x, 
                               g_state.colony->y,
                               g_state.colony->radius);
            break;
            
        case SDLK_PERIOD:  /* . = speed up */
            if (g_state.speed_index < SPEED_LEVELS_COUNT - 1) {
                g_state.speed_index++;
                g_state.sim_speed = SPEED_LEVELS[g_state.speed_index];
                printf("Speed: %.2fx\n", g_state.sim_speed);
            }
            break;
            
        case SDLK_COMMA:  /* , = slow down */
            if (g_state.speed_index > 0) {
                g_state.speed_index--;
                g_state.sim_speed = SPEED_LEVELS[g_state.speed_index];
                printf("Speed: %.2fx\n", g_state.sim_speed);
            }
            break;
    }
}

static void handle_mouse_click(int x, int y, bool f_held) {
    if (f_held) {
        /* Add food at click position */
        if (colony_add_food_source(g_state.colony, (float)x, (float)y, CLICK_FOOD_AMOUNT)) {
            printf("Added food at (%d, %d)\n", x, y);
        }
    }
}

static void handle_events(void) {
    SDL_Event event;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    bool f_held = keys[SDL_SCANCODE_F];
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_state.running = false;
                break;
                
            case SDL_KEYDOWN:
                handle_keydown(event.key.keysym.sym);
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handle_mouse_click(event.button.x, event.button.y, f_held);
                }
                break;
        }
    }
}

/* ============== Main Loop ============== */

static void update(void) {
    if (g_state.paused) return;
    
    /* Run simulation steps based on speed */
    int steps = (int)g_state.sim_speed;
    if (steps < 1) steps = 1;
    
    /* Handle fractional speeds */
    static float accumulator = 0.0f;
    if (g_state.sim_speed < 1.0f) {
        accumulator += g_state.sim_speed;
        if (accumulator >= 1.0f) {
            accumulator -= 1.0f;
            colony_update(g_state.colony);
        }
    } else {
        for (int i = 0; i < steps; i++) {
            colony_update(g_state.colony);
        }
    }
}

static void render(SDL_Renderer *renderer) {
    render_clear(renderer);
    
    /* Grid (behind everything) */
    if (g_state.show_grid) {
        render_grid(renderer, g_state.screen_width, g_state.screen_height);
    }
    
    /* Pheromones (below ants) */
    if (g_state.show_pheromones) {
        render_pheromones(renderer, g_state.colony);
    }
    
    /* Walls */
    render_walls(renderer, g_state.colony);
    
    /* Death markers */
    render_death_markers(renderer, g_state.colony);
    
    /* Food sources */
    render_food_sources(renderer, g_state.colony);
    
    /* Colony */
    render_colony(renderer, g_state.colony);
    
    /* Ants */
    render_ants(renderer, g_state.colony);
    
    /* HUD */
    render_hud(renderer, &g_state);
    
    /* Hints overlay */
    if (g_state.show_hints) {
        render_hints(renderer);
    }
    
    render_present(renderer);
}

/* ============== Entry Point ============== */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("=== Ant Colony Simulator (C) ===\n");
    printf("Initializing...\n");
    
    /* Seed random number generator */
    rng_seed((uint32_t)time(NULL));
    
    /* Initialize rendering */
    SDL_Renderer *renderer = render_init(&g_state.screen_width, &g_state.screen_height);
    if (!renderer) {
        fprintf(stderr, "Failed to initialize rendering: %s\n", SDL_GetError());
        return 1;
    }
    
    printf("Screen: %dx%d\n", g_state.screen_width, g_state.screen_height);
    
    /* Initialize simulation state */
    g_state.running = true;
    g_state.paused = false;
    g_state.show_pheromones = false;
    g_state.show_grid = true;
    g_state.show_neural_ui = true;
    g_state.show_hints = false;
    g_state.show_debug = false;
    g_state.speed_index = DEFAULT_SPEED_INDEX;
    g_state.sim_speed = SPEED_LEVELS[g_state.speed_index];
    
    /* Create colony */
    Rect bounds = {0, 0, g_state.screen_width, g_state.screen_height};
    g_state.colony = colony_create(
        (float)(g_state.screen_width / 2),
        (float)(g_state.screen_height / 2),
        g_state.screen_width,
        g_state.screen_height,
        bounds
    );
    
    if (!g_state.colony) {
        fprintf(stderr, "Failed to create colony\n");
        render_cleanup(renderer);
        return 1;
    }
    
    printf("Colony created at (%d, %d)\n", 
           g_state.screen_width / 2, g_state.screen_height / 2);
    printf("Initial population: %d ants\n", g_state.colony->ant_count);
    printf("\nControls:\n");
    printf("  SPACE   - Pause/Resume\n");
    printf("  P       - Toggle pheromones\n");
    printf("  G       - Toggle grid\n");
    printf("  R       - Reset colony\n");
    printf("  M       - New maze\n");
    printf("  ,/.     - Speed down/up\n");
    printf("  F+Click - Add food\n");
    printf("  ESC     - Quit\n\n");
    
    /* Main loop */
    Uint32 last_time = SDL_GetTicks();
    Uint32 frame_count = 0;
    Uint32 fps_timer = last_time;
    
    while (g_state.running) {
        Uint32 current_time = SDL_GetTicks();
        
        handle_events();
        update();
        render(renderer);
        
        frame_count++;
        
        /* FPS counter every second */
        if (current_time - fps_timer >= 1000) {
            int pop, gen;
            float food, fitness;
            colony_get_stats(g_state.colony, &pop, &food, &gen, &fitness);
            printf("\rFPS: %u | Pop: %d | Food: %.0f | Gen: %d | Best: %.1f    ",
                   frame_count, pop, food, gen, fitness);
            fflush(stdout);
            frame_count = 0;
            fps_timer = current_time;
        }
        
        /* Frame limiting */
        Uint32 frame_time = SDL_GetTicks() - current_time;
        if (frame_time < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - frame_time);
        }
        
        last_time = current_time;
    }
    
    printf("\n\nShutting down...\n");
    
    /* Cleanup */
    colony_destroy(g_state.colony);
    render_cleanup(renderer);
    
    printf("Goodbye!\n");
    return 0;
}
