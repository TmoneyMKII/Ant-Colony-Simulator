/**
 * @file render.c
 * @brief SDL2 rendering implementation
 */

#include "render.h"
#include "pheromone.h"
#include "walls.h"
#include "config.h"
#include <math.h>

static SDL_Window *g_window = NULL;

/* ============== Initialization ============== */

SDL_Renderer* render_init(int *width, int *height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return NULL;
    }
    
    /* Get display size */
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        SDL_Quit();
        return NULL;
    }
    
    *width = dm.w;
    *height = dm.h;
    
    /* Create fullscreen window */
    g_window = SDL_CreateWindow(WINDOW_TITLE,
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                dm.w, dm.h,
                                SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    if (!g_window) {
        SDL_Quit();
        return NULL;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(g_window, -1,
                                                 SDL_RENDERER_ACCELERATED | 
                                                 SDL_RENDERER_PRESENTVSYNC);
    
    if (!renderer) {
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return NULL;
    }
    
    /* Enable alpha blending */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    return renderer;
}

void render_cleanup(SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void render_clear(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, COLOR_BG_A);
    SDL_RenderClear(renderer);
}

void render_present(SDL_Renderer *renderer) {
    SDL_RenderPresent(renderer);
}

/* ============== Primitives ============== */

void render_circle(SDL_Renderer *renderer, int cx, int cy, int radius, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    /* Midpoint circle algorithm with horizontal lines for fill */
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
        SDL_RenderDrawLine(renderer, cx - x, cy - y, cx + x, cy - y);
        SDL_RenderDrawLine(renderer, cx - y, cy + x, cx + y, cy + x);
        SDL_RenderDrawLine(renderer, cx - y, cy - x, cx + y, cy - x);
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void render_rect(SDL_Renderer *renderer, int x, int y, int w, int h, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

/* ============== Game Objects ============== */

void render_colony(SDL_Renderer *renderer, const Colony *colony) {
    render_circle(renderer, (int)colony->x, (int)colony->y, 
                  (int)colony->radius, colony->color);
    
    /* Draw entrance indicator */
    Color entrance_color = {180, 140, 90, 255};
    render_circle(renderer, (int)colony->x, (int)colony->y, 
                  (int)(colony->radius * 0.4f), entrance_color);
}

void render_ants(SDL_Renderer *renderer, const Colony *colony) {
    for (int i = 0; i < colony->ant_count; i++) {
        const Ant *ant = &colony->ants[i];
        if (!ant->alive) continue;
        
        /* Color based on state */
        Color color;
        if (ant->carrying_food) {
            color = (Color){100, 255, 150, 255};  /* Green when carrying */
        } else {
            color = ant->color;
        }
        
        /* Draw ant body */
        render_circle(renderer, (int)ant->x, (int)ant->y, 
                      (int)ant->radius, color);
        
        /* Draw direction indicator */
        int dx = (int)(cosf(ant->direction) * ant->radius * 1.5f);
        int dy = (int)(sinf(ant->direction) * ant->radius * 1.5f);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
        SDL_RenderDrawLine(renderer, (int)ant->x, (int)ant->y,
                          (int)ant->x + dx, (int)ant->y + dy);
    }
}

void render_food_sources(SDL_Renderer *renderer, const Colony *colony) {
    for (int i = 0; i < colony->food_count; i++) {
        const FoodSource *food = &colony->food_sources[i];
        if (food->amount <= 0) continue;
        
        /* Size based on remaining food */
        float size_ratio = fmaxf(0.3f, food->amount / food->max_amount);
        int current_radius = (int)(food->radius * size_ratio);
        
        /* Color intensity based on amount */
        int intensity = (int)(200 * size_ratio);
        Color color = {(uint8_t)intensity, (uint8_t)(150 * size_ratio), 50, 255};
        
        render_circle(renderer, (int)food->x, (int)food->y, 
                      current_radius, color);
    }
}

void render_walls(SDL_Renderer *renderer, const Colony *colony) {
    Color wall_color = {COLOR_WALL_R, COLOR_WALL_G, COLOR_WALL_B, 255};
    
    int wall_count = walls_get_count(colony->wall_manager);
    for (int i = 0; i < wall_count; i++) {
        const Rect *wall = walls_get(colony->wall_manager, i);
        if (wall) {
            render_rect(renderer, wall->x, wall->y, wall->width, wall->height, wall_color);
        }
    }
}

void render_death_markers(SDL_Renderer *renderer, const Colony *colony) {
    for (int i = 0; i < colony->death_marker_count; i++) {
        const DeathMarker *marker = &colony->death_markers[i];
        
        /* Fade out as marker expires */
        float alpha_ratio = (float)marker->frames_remaining / DEATH_MARKER_DURATION;
        uint8_t alpha = (uint8_t)(180 * alpha_ratio);
        
        Color color = {150, 30, 30, alpha};
        render_circle(renderer, (int)marker->x, (int)marker->y, 8, color);
    }
}

void render_pheromones(SDL_Renderer *renderer, const Colony *colony) {
    const PheromoneMap *map = colony->pheromone_map;
    int grid_w, grid_h;
    pheromone_get_grid_size(map, &grid_w, &grid_h);
    
    int cell_size = PHEROMONE_CELL_SIZE;
    
    for (int gy = 0; gy < grid_h; gy++) {
        for (int gx = 0; gx < grid_w; gx++) {
            float food_val = pheromone_get_cell(map, gx, gy, PHEROMONE_FOOD_TRAIL);
            float home_val = pheromone_get_cell(map, gx, gy, PHEROMONE_HOME_TRAIL);
            float danger_val = pheromone_get_cell(map, gx, gy, PHEROMONE_DANGER);
            
            int x = gx * cell_size;
            int y = gy * cell_size;
            
            /* Draw food trail (green) */
            if (food_val > 5.0f) {
                uint8_t alpha = (uint8_t)fminf(150, food_val * 0.75f);
                Color color = {0, 255, 100, alpha};
                render_rect(renderer, x, y, cell_size, cell_size, color);
            }
            
            /* Draw home trail (blue) */
            if (home_val > 5.0f) {
                uint8_t alpha = (uint8_t)fminf(150, home_val * 0.75f);
                Color color = {100, 150, 255, alpha};
                render_rect(renderer, x, y, cell_size, cell_size, color);
            }
            
            /* Draw danger (red) */
            if (danger_val > 5.0f) {
                uint8_t alpha = (uint8_t)fminf(150, danger_val * 0.75f);
                Color color = {255, 50, 50, alpha};
                render_rect(renderer, x, y, cell_size, cell_size, color);
            }
        }
    }
}

void render_grid(SDL_Renderer *renderer, int width, int height) {
    SDL_SetRenderDrawColor(renderer, COLOR_GRID_R, COLOR_GRID_G, COLOR_GRID_B, 100);
    
    for (int x = 0; x < width; x += GRID_CELL_SIZE) {
        SDL_RenderDrawLine(renderer, x, 0, x, height);
    }
    for (int y = 0; y < height; y += GRID_CELL_SIZE) {
        SDL_RenderDrawLine(renderer, 0, y, width, y);
    }
}

void render_hud(SDL_Renderer *renderer, const SimState *state) {
    /* Simple status bar at top */
    int bar_height = 30;
    Color bar_color = {0, 0, 0, 180};
    render_rect(renderer, 0, 0, state->screen_width, bar_height, bar_color);
    
    /* Note: Full text rendering would require SDL_ttf
       For now, we just show colored indicators */
    
    int x = 10;
    int y = 8;
    int box_size = 14;
    int spacing = 80;
    
    /* Population indicator (green) */
    Color pop_color = {100, 255, 150, 255};
    render_rect(renderer, x, y, box_size, box_size, pop_color);
    
    /* Food indicator (yellow) */
    x += spacing;
    Color food_color = {255, 200, 50, 255};
    render_rect(renderer, x, y, box_size, box_size, food_color);
    
    /* Speed indicator */
    x += spacing;
    Color speed_color = {255, 255, 255, 255};
    int speed_width = (int)(box_size * state->sim_speed);
    render_rect(renderer, x, y, speed_width, box_size, speed_color);
    
    /* Pause indicator */
    if (state->paused) {
        x += spacing;
        Color pause_color = {255, 100, 100, 255};
        render_rect(renderer, x, y, box_size / 3, box_size, pause_color);
        render_rect(renderer, x + box_size / 2, y, box_size / 3, box_size, pause_color);
    }
}

void render_hints(SDL_Renderer *renderer) {
    /* Semi-transparent hint box */
    Color box_color = {0, 0, 0, 200};
    render_rect(renderer, 20, 50, 200, 280, box_color);
    
    /* Would need SDL_ttf for actual text rendering */
    /* For now just show it's the hint area */
    Color hint_color = {200, 200, 210, 255};
    render_rect(renderer, 30, 60, 180, 2, hint_color);
}
