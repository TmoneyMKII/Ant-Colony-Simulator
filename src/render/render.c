/**
 * @file render.c
 * @brief World rendering
 *
 * Two techniques keep the cost flat as the colony grows:
 *   - Ants, food and markers are quads in one vertex buffer per kind,
 *     submitted as a single SDL_RenderGeometry call against a baked sprite.
 *   - Pheromones are uploaded as one small texture (a pixel per grid cell)
 *     and stretched over the world with linear filtering, instead of one
 *     filled rectangle per cell per layer.
 */

#include "render.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#define ANT_SPRITE_SIZE   64
#define DISC_SPRITE_SIZE  64
#define GRID_MIN_PIXELS   6.0f      /* hide the grid when cells get this small */
#define TRAIL_FULL_VALUE  120.0f    /* pheromone value that renders at full strength */

/* ============== Palette ============== */

static const SDL_Color COLOR_VOID       = {10, 10, 13, 255};
static const SDL_Color COLOR_GROUND     = {18, 18, 24, 255};
static const SDL_Color COLOR_GRID       = {38, 38, 48, 255};
static const SDL_Color COLOR_WALL       = {95, 55, 120, 255};
static const SDL_Color COLOR_WALL_EDGE  = {135, 80, 165, 255};
static const SDL_Color COLOR_ANT        = {222, 138, 188, 255};
static const SDL_Color COLOR_ANT_LADEN  = {110, 235, 150, 255};
static const SDL_Color COLOR_SELECTED   = {100, 200, 255, 255};
static const SDL_Color COLOR_FOOD       = {235, 170, 60, 255};
static const SDL_Color COLOR_NEST       = {170, 120, 70, 255};
static const SDL_Color COLOR_NEST_INNER = {205, 155, 100, 255};
static const SDL_Color COLOR_MARKER     = {175, 50, 50, 255};
static const SDL_Color COLOR_TRAIL_FOOD = {60, 220, 130, 255};
static const SDL_Color COLOR_TRAIL_HOME = {90, 150, 255, 255};
static const SDL_Color COLOR_DANGER     = {235, 70, 70, 255};
static const SDL_Color COLOR_RAY_NONE   = {70, 70, 85, 255};

/* ============== Vertex batch ============== */

typedef struct {
    SDL_Vertex *vertices;
    int *indices;
    int quad_count;
    int capacity;        /**< In quads */
} QuadBatch;

static bool batch_reserve(QuadBatch *b, int quads) {
    if (quads <= b->capacity) return true;
    int cap = b->capacity ? b->capacity : 256;
    while (cap < quads) cap *= 2;

    SDL_Vertex *v = realloc(b->vertices, sizeof(SDL_Vertex) * (size_t)cap * 4);
    int *idx = realloc(b->indices, sizeof(int) * (size_t)cap * 6);
    if (!v || !idx) {
        /* Keep whatever grew; the draw will simply be capped */
        if (v) b->vertices = v;
        if (idx) b->indices = idx;
        return false;
    }
    b->vertices = v;
    b->indices = idx;
    b->capacity = cap;
    return true;
}

static void batch_free(QuadBatch *b) {
    free(b->vertices);
    free(b->indices);
    memset(b, 0, sizeof(*b));
}

static void batch_clear(QuadBatch *b) {
    b->quad_count = 0;
}

/** @brief Add a rotated, coloured quad covering the whole sprite texture */
static void batch_push(QuadBatch *b, Vec2 center, float half_w, float half_h,
                       float cos_a, float sin_a, SDL_Color color) {
    if (b->quad_count >= b->capacity && !batch_reserve(b, b->quad_count + 1)) return;

    const float xs[4] = {-half_w, half_w, half_w, -half_w};
    const float ys[4] = {-half_h, -half_h, half_h, half_h};
    const float us[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    const float vs[4] = {0.0f, 0.0f, 1.0f, 1.0f};

    int vi = b->quad_count * 4;
    for (int i = 0; i < 4; i++) {
        b->vertices[vi + i].position.x = center.x + xs[i] * cos_a - ys[i] * sin_a;
        b->vertices[vi + i].position.y = center.y + xs[i] * sin_a + ys[i] * cos_a;
        b->vertices[vi + i].color = color;
        b->vertices[vi + i].tex_coord.x = us[i];
        b->vertices[vi + i].tex_coord.y = vs[i];
    }

    int ii = b->quad_count * 6;
    b->indices[ii + 0] = vi + 0;
    b->indices[ii + 1] = vi + 1;
    b->indices[ii + 2] = vi + 2;
    b->indices[ii + 3] = vi + 0;
    b->indices[ii + 4] = vi + 2;
    b->indices[ii + 5] = vi + 3;

    b->quad_count++;
}

static void batch_flush(QuadBatch *b, SDL_Renderer *renderer, SDL_Texture *texture) {
    if (b->quad_count == 0) return;
    SDL_RenderGeometry(renderer, texture, b->vertices, b->quad_count * 4,
                       b->indices, b->quad_count * 6);
    batch_clear(b);
}

/* ============== Renderer ============== */

struct WorldRenderer {
    SDL_Texture *ant_sprite;
    SDL_Texture *disc_sprite;

    SDL_Texture *trails;        /**< One pixel per pheromone cell */
    uint32_t *trail_pixels;
    int trail_cols;
    int trail_rows;

    QuadBatch batch;
    int last_ant_count;
};

/* ---- Sprite baking ---- */

static inline float ellipse_coverage(float px, float py, float cx, float cy, float rx, float ry) {
    float dx = (px - cx) / rx;
    float dy = (py - cy) / ry;
    return dx * dx + dy * dy <= 1.0f ? 1.0f : 0.0f;
}

/** @brief Distance from a point to a segment, for drawing legs and antennae */
static float point_segment_dist(float px, float py, float x0, float y0, float x1, float y1) {
    float vx = x1 - x0, vy = y1 - y0;
    float wx = px - x0, wy = py - y0;
    float len_sq = vx * vx + vy * vy;
    float t = len_sq > 0.0f ? clampf((wx * vx + wy * vy) / len_sq, 0.0f, 1.0f) : 0.0f;
    float dx = wx - vx * t, dy = wy - vy * t;
    return sqrtf(dx * dx + dy * dy);
}

/**
 * @brief Is this point inside the ant silhouette?
 *
 * Coordinates are normalised to [0,1] with the ant facing +x: abdomen,
 * thorax and head as ellipses, plus three pairs of legs and antennae.
 */
static bool inside_ant(float x, float y) {
    if (ellipse_coverage(x, y, 0.28f, 0.5f, 0.20f, 0.150f) > 0.0f) return true;
    if (ellipse_coverage(x, y, 0.55f, 0.5f, 0.12f, 0.105f) > 0.0f) return true;
    if (ellipse_coverage(x, y, 0.77f, 0.5f, 0.13f, 0.120f) > 0.0f) return true;

    /* Legs: three pairs splaying from the thorax */
    static const float legs[6][4] = {
        {0.52f, 0.44f, 0.38f, 0.10f}, {0.52f, 0.56f, 0.38f, 0.90f},
        {0.56f, 0.43f, 0.56f, 0.06f}, {0.56f, 0.57f, 0.56f, 0.94f},
        {0.60f, 0.44f, 0.76f, 0.12f}, {0.60f, 0.56f, 0.76f, 0.88f},
    };
    for (int i = 0; i < 6; i++) {
        if (point_segment_dist(x, y, legs[i][0], legs[i][1], legs[i][2], legs[i][3]) < 0.022f) {
            return true;
        }
    }

    /* Antennae */
    if (point_segment_dist(x, y, 0.86f, 0.45f, 0.99f, 0.28f) < 0.018f) return true;
    if (point_segment_dist(x, y, 0.86f, 0.55f, 0.99f, 0.72f) < 0.018f) return true;

    return false;
}

/** @brief Bake a white sprite with antialiased alpha from a coverage test */
static SDL_Texture *bake_sprite(SDL_Renderer *renderer, int size, bool is_ant) {
    uint32_t *pixels = malloc(sizeof(uint32_t) * (size_t)size * (size_t)size);
    if (!pixels) return NULL;

    const int samples = 4;  /* 4x4 supersampling */
    for (int py = 0; py < size; py++) {
        for (int px = 0; px < size; px++) {
            int hits = 0;
            for (int sy = 0; sy < samples; sy++) {
                for (int sx = 0; sx < samples; sx++) {
                    float x = ((float)px + ((float)sx + 0.5f) / samples) / (float)size;
                    float y = ((float)py + ((float)sy + 0.5f) / samples) / (float)size;
                    bool in;
                    if (is_ant) {
                        in = inside_ant(x, y);
                    } else {
                        float dx = x - 0.5f, dy = y - 0.5f;
                        in = dx * dx + dy * dy <= 0.25f;
                    }
                    if (in) hits++;
                }
            }
            uint32_t alpha = (uint32_t)(255 * hits / (samples * samples));
            pixels[py * size + px] = (alpha << 24) | 0x00FFFFFFu;
        }
    }

    SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STATIC, size, size);
    if (tex) {
        SDL_UpdateTexture(tex, NULL, pixels, size * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
    }
    free(pixels);
    return tex;
}

WorldRenderer *world_renderer_create(SDL_Renderer *renderer) {
    WorldRenderer *wr = calloc(1, sizeof(WorldRenderer));
    if (!wr) return NULL;

    wr->ant_sprite = bake_sprite(renderer, ANT_SPRITE_SIZE, true);
    wr->disc_sprite = bake_sprite(renderer, DISC_SPRITE_SIZE, false);
    if (!wr->ant_sprite || !wr->disc_sprite || !batch_reserve(&wr->batch, 1024)) {
        world_renderer_destroy(wr);
        return NULL;
    }
    return wr;
}

void world_renderer_destroy(WorldRenderer *wr) {
    if (!wr) return;
    if (wr->ant_sprite) SDL_DestroyTexture(wr->ant_sprite);
    if (wr->disc_sprite) SDL_DestroyTexture(wr->disc_sprite);
    if (wr->trails) SDL_DestroyTexture(wr->trails);
    free(wr->trail_pixels);
    batch_free(&wr->batch);
    free(wr);
}

int world_renderer_last_ant_count(const WorldRenderer *wr) {
    return wr ? wr->last_ant_count : 0;
}

void render_options_defaults(RenderOptions *opt) {
    memset(opt, 0, sizeof(*opt));
    opt->grid = true;
    opt->walls = true;
    opt->food = true;
    opt->nest = true;
    opt->ants = true;
    opt->markers = true;
    opt->ant_headings = false;
    opt->trail_food = false;
    opt->trail_home = false;
    opt->trail_danger = false;
    opt->vision_rays = true;
}

/* ============== Camera ============== */

void camera_init(Camera *cam, SDL_Rect viewport, int world_width, int world_height) {
    cam->viewport = viewport;
    cam->min_zoom = 0.05f;
    cam->max_zoom = 8.0f;
    camera_fit(cam, world_width, world_height);
}

void camera_fit(Camera *cam, int world_width, int world_height) {
    float zx = (float)cam->viewport.w / (float)world_width;
    float zy = (float)cam->viewport.h / (float)world_height;
    float zoom = fminf(zx, zy);

    cam->min_zoom = zoom * 0.5f;
    cam->zoom = zoom;
    cam->center = vec2((float)world_width * 0.5f, (float)world_height * 0.5f);
}

Vec2 camera_to_screen(const Camera *cam, Vec2 world) {
    return vec2((float)cam->viewport.x + (float)cam->viewport.w * 0.5f +
                    (world.x - cam->center.x) * cam->zoom,
                (float)cam->viewport.y + (float)cam->viewport.h * 0.5f +
                    (world.y - cam->center.y) * cam->zoom);
}

Vec2 camera_to_world(const Camera *cam, Vec2 screen) {
    return vec2(cam->center.x +
                    (screen.x - (float)cam->viewport.x - (float)cam->viewport.w * 0.5f) / cam->zoom,
                cam->center.y +
                    (screen.y - (float)cam->viewport.y - (float)cam->viewport.h * 0.5f) / cam->zoom);
}

void camera_pan(Camera *cam, float dx, float dy) {
    cam->center.x -= dx / cam->zoom;
    cam->center.y -= dy / cam->zoom;
}

void camera_zoom_at(Camera *cam, Vec2 anchor_screen, float factor) {
    Vec2 before = camera_to_world(cam, anchor_screen);
    cam->zoom = clampf(cam->zoom * factor, cam->min_zoom, cam->max_zoom);
    Vec2 after = camera_to_world(cam, anchor_screen);
    cam->center = vec2_add(cam->center, vec2_sub(before, after));
}

void camera_clamp(Camera *cam, int world_width, int world_height) {
    cam->zoom = clampf(cam->zoom, cam->min_zoom, cam->max_zoom);

    /* Let the view drift a quarter-screen past the edge, no further, so the
       world can never be panned entirely out of sight. */
    float slack_x = (float)cam->viewport.w * 0.25f / cam->zoom;
    float slack_y = (float)cam->viewport.h * 0.25f / cam->zoom;

    cam->center.x = clampf(cam->center.x, -slack_x, (float)world_width + slack_x);
    cam->center.y = clampf(cam->center.y, -slack_y, (float)world_height + slack_y);
}

/* ============== Drawing helpers ============== */

static inline void set_color(SDL_Renderer *r, SDL_Color c, uint8_t alpha) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, alpha);
}

static SDL_FRect world_rect_to_screen(const Camera *cam, Rect r) {
    Vec2 tl = camera_to_screen(cam, vec2((float)r.x, (float)r.y));
    Vec2 br = camera_to_screen(cam, vec2((float)(r.x + r.width), (float)(r.y + r.height)));
    return (SDL_FRect){tl.x, tl.y, br.x - tl.x, br.y - tl.y};
}

static void draw_grid(SDL_Renderer *renderer, const Camera *cam, const World *w) {
    float step = (float)PHEROMONE_CELL_SIZE * cam->zoom;
    if (step < GRID_MIN_PIXELS) return;

    set_color(renderer, COLOR_GRID, 120);
    Vec2 origin = camera_to_screen(cam, vec2(0, 0));
    Vec2 far = camera_to_screen(cam, vec2((float)w->width, (float)w->height));

    for (float x = origin.x; x <= far.x; x += step) {
        SDL_RenderDrawLineF(renderer, x, origin.y, x, far.y);
    }
    for (float y = origin.y; y <= far.y; y += step) {
        SDL_RenderDrawLineF(renderer, origin.x, y, far.x, y);
    }
}

/** @brief Upload the enabled pheromone layers as one texture and stretch it */
static void draw_trails(WorldRenderer *wr, SDL_Renderer *renderer, const World *w,
                        const Camera *cam, const RenderOptions *opt) {
    if (!opt->trail_food && !opt->trail_home && !opt->trail_danger) return;

    const PheromoneMap *map = &w->pheromones;
    int cols = map->cols, rows = map->rows;

    if (wr->trail_cols != cols || wr->trail_rows != rows) {
        if (wr->trails) SDL_DestroyTexture(wr->trails);
        free(wr->trail_pixels);
        wr->trails = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING, cols, rows);
        wr->trail_pixels = malloc(sizeof(uint32_t) * (size_t)cols * (size_t)rows);
        wr->trail_cols = cols;
        wr->trail_rows = rows;
        if (!wr->trails || !wr->trail_pixels) return;
        SDL_SetTextureScaleMode(wr->trails, SDL_ScaleModeLinear);
        SDL_SetTextureBlendMode(wr->trails, SDL_BLENDMODE_ADD);
    }
    if (!wr->trails || !wr->trail_pixels) return;

    const float *food = pheromone_layer(map, PHEROMONE_FOOD);
    const float *home = pheromone_layer(map, PHEROMONE_HOME);
    const float *danger = pheromone_layer(map, PHEROMONE_DANGER);

    for (int i = 0; i < cols * rows; i++) {
        float wf = opt->trail_food ? clampf(food[i] / TRAIL_FULL_VALUE, 0.0f, 1.0f) : 0.0f;
        float wh = opt->trail_home ? clampf(home[i] / TRAIL_FULL_VALUE, 0.0f, 1.0f) : 0.0f;
        float wd = opt->trail_danger ? clampf(danger[i] / TRAIL_FULL_VALUE, 0.0f, 1.0f) : 0.0f;

        float total = wf + wh + wd;
        if (total <= 0.001f) {
            wr->trail_pixels[i] = 0;
            continue;
        }

        /* Blend the layer colours by strength; alpha carries the intensity */
        float r = (COLOR_TRAIL_FOOD.r * wf + COLOR_TRAIL_HOME.r * wh + COLOR_DANGER.r * wd) / total;
        float g = (COLOR_TRAIL_FOOD.g * wf + COLOR_TRAIL_HOME.g * wh + COLOR_DANGER.g * wd) / total;
        float b = (COLOR_TRAIL_FOOD.b * wf + COLOR_TRAIL_HOME.b * wh + COLOR_DANGER.b * wd) / total;
        uint32_t a = (uint32_t)(clampf(total, 0.0f, 1.0f) * 200.0f);

        wr->trail_pixels[i] = (a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    SDL_UpdateTexture(wr->trails, NULL, wr->trail_pixels, cols * (int)sizeof(uint32_t));

    SDL_FRect dst = world_rect_to_screen(cam, (Rect){0, 0,
                                                    cols * map->cell_size,
                                                    rows * map->cell_size});
    SDL_RenderCopyF(renderer, wr->trails, NULL, &dst);
}

static void draw_walls(SDL_Renderer *renderer, const World *w, const Camera *cam) {
    for (int i = 0; i < w->walls.count; i++) {
        SDL_FRect r = world_rect_to_screen(cam, w->walls.rects[i]);
        set_color(renderer, COLOR_WALL, 255);
        SDL_RenderFillRectF(renderer, &r);
        set_color(renderer, COLOR_WALL_EDGE, 255);
        SDL_RenderDrawRectF(renderer, &r);
    }
}

static void draw_discs(WorldRenderer *wr, SDL_Renderer *renderer, const World *w,
                       const Camera *cam, const RenderOptions *opt) {
    batch_clear(&wr->batch);

    if (opt->markers) {
        for (int i = 0; i < w->marker_count; i++) {
            const DeathMarker *m = &w->markers[i];
            float fade = (float)m->ttl / (float)DEATH_MARKER_DURATION;
            SDL_Color c = COLOR_MARKER;
            c.a = (uint8_t)(170.0f * fade);
            float radius = 7.0f * cam->zoom;
            batch_push(&wr->batch, camera_to_screen(cam, m->pos), radius, radius, 1.0f, 0.0f, c);
        }
    }

    if (opt->food) {
        for (int i = 0; i < w->food_count; i++) {
            const FoodSource *f = &w->food[i];
            if (f->amount <= 0.0f) continue;
            float ratio = f->max_amount > 0.0f ? f->amount / f->max_amount : 0.0f;
            float radius = FOOD_RADIUS * fmaxf(0.35f, ratio) * cam->zoom;
            SDL_Color c = COLOR_FOOD;
            c.a = (uint8_t)(180 + 75 * clampf(ratio, 0.0f, 1.0f));
            batch_push(&wr->batch, camera_to_screen(cam, f->pos), radius, radius, 1.0f, 0.0f, c);
        }
    }

    if (opt->nest) {
        float radius = w->nest.radius * cam->zoom;
        Vec2 center = camera_to_screen(cam, w->nest.pos);
        batch_push(&wr->batch, center, radius, radius, 1.0f, 0.0f, COLOR_NEST);
        batch_push(&wr->batch, center, radius * 0.45f, radius * 0.45f, 1.0f, 0.0f, COLOR_NEST_INNER);
    }

    batch_flush(&wr->batch, renderer, wr->disc_sprite);
}

static void draw_ants(WorldRenderer *wr, SDL_Renderer *renderer, const World *w,
                      const Camera *cam, const RenderOptions *opt, int selected) {
    if (!opt->ants) {
        wr->last_ant_count = 0;
        return;
    }
    batch_clear(&wr->batch);

    /* Cull to the viewport with a margin for sprites straddling the edge */
    Vec2 top_left = camera_to_world(cam, vec2((float)cam->viewport.x, (float)cam->viewport.y));
    Vec2 bottom_right = camera_to_world(cam, vec2((float)(cam->viewport.x + cam->viewport.w),
                                                 (float)(cam->viewport.y + cam->viewport.h)));
    const float margin = ANT_RADIUS * 3.0f;

    float half_len = ANT_RADIUS * 1.45f * cam->zoom;
    float half_wid = ANT_RADIUS * 0.95f * cam->zoom;
    if (half_len < 1.5f) half_len = 1.5f;   /* stay visible when zoomed out */
    if (half_wid < 1.0f) half_wid = 1.0f;

    int drawn = 0;
    for (int i = 0; i < w->ant_count; i++) {
        const Ant *a = &w->ants[i];
        if (a->pos.x < top_left.x - margin || a->pos.x > bottom_right.x + margin ||
            a->pos.y < top_left.y - margin || a->pos.y > bottom_right.y + margin) {
            continue;
        }

        SDL_Color color = (a->state == ANT_STATE_RETURNING) ? COLOR_ANT_LADEN : COLOR_ANT;
        if (i == selected) color = COLOR_SELECTED;

        batch_push(&wr->batch, camera_to_screen(cam, a->pos), half_len, half_wid,
                   cosf(a->heading), sinf(a->heading), color);
        drawn++;
    }
    batch_flush(&wr->batch, renderer, wr->ant_sprite);
    wr->last_ant_count = drawn;

    if (opt->ant_headings) {
        set_color(renderer, COLOR_SELECTED, 90);
        for (int i = 0; i < w->ant_count; i++) {
            const Ant *a = &w->ants[i];
            Vec2 from = camera_to_screen(cam, a->pos);
            Vec2 to = camera_to_screen(cam, vec2_add(a->pos,
                          vec2_scale(vec2_from_angle(a->heading), ANT_RADIUS * 3.0f)));
            SDL_RenderDrawLineF(renderer, from.x, from.y, to.x, to.y);
        }
    }
}

/** @brief Highlight ring plus vision rays for the inspected ant */
static void draw_selection(SDL_Renderer *renderer, const World *w, const Camera *cam,
                           const RenderOptions *opt, int selected) {
    if (selected < 0 || selected >= w->ant_count) return;
    const Ant *a = &w->ants[selected];
    Vec2 center = camera_to_screen(cam, a->pos);

    /* Ring, drawn as a 24-segment polyline */
    float radius = fmaxf(10.0f, ANT_RADIUS * 2.2f * cam->zoom);
    set_color(renderer, COLOR_SELECTED, 220);
    const int segments = 24;
    for (int i = 0; i < segments; i++) {
        float a0 = TWO_PI_F * (float)i / segments;
        float a1 = TWO_PI_F * (float)(i + 1) / segments;
        SDL_RenderDrawLineF(renderer,
                            center.x + cosf(a0) * radius, center.y + sinf(a0) * radius,
                            center.x + cosf(a1) * radius, center.y + sinf(a1) * radius);
    }

    if (!opt->vision_rays || w->cfg.brain_mode != BRAIN_NEURAL) return;

    for (int r = 0; r < NN_NUM_VISION_RAYS; r++) {
        const VisionRay *ray = &a->vision[r];

        /* Draw to the nearest hit, coloured by what was hit */
        float closeness = fmaxf(fmaxf(ray->wall, ray->ant), ray->food);
        float length = (1.0f - closeness) * VISION_RAY_LENGTH;
        SDL_Color color = COLOR_RAY_NONE;
        uint8_t alpha = 90;
        if (closeness > 0.001f) {
            if (ray->wall >= ray->ant && ray->wall >= ray->food) color = COLOR_WALL_EDGE;
            else if (ray->food >= ray->ant) color = COLOR_TRAIL_FOOD;
            else color = COLOR_FOOD;
            alpha = 200;
        } else {
            length = VISION_RAY_LENGTH;
        }

        Vec2 dir = vec2_from_angle(a->heading + vision_ray_angle(r));
        Vec2 tip = camera_to_screen(cam, vec2_add(a->pos, vec2_scale(dir, length)));
        set_color(renderer, color, alpha);
        SDL_RenderDrawLineF(renderer, center.x, center.y, tip.x, tip.y);
    }
}

/* ============== Entry point ============== */

void world_renderer_draw(WorldRenderer *wr, SDL_Renderer *renderer, const World *w,
                         const Camera *cam, const RenderOptions *opt, int selected) {
    /* Outside the world, then the ground it occupies */
    set_color(renderer, COLOR_VOID, 255);
    SDL_RenderClear(renderer);

    SDL_Rect clip = cam->viewport;
    SDL_RenderSetClipRect(renderer, &clip);

    SDL_FRect ground = world_rect_to_screen(cam, (Rect){0, 0, w->width, w->height});
    set_color(renderer, COLOR_GROUND, 255);
    SDL_RenderFillRectF(renderer, &ground);

    if (opt->grid) draw_grid(renderer, cam, w);
    draw_trails(wr, renderer, w, cam, opt);
    if (opt->walls) draw_walls(renderer, w, cam);
    draw_discs(wr, renderer, w, cam, opt);
    draw_ants(wr, renderer, w, cam, opt, selected);
    draw_selection(renderer, w, cam, opt, selected);

    /* World border */
    set_color(renderer, COLOR_WALL_EDGE, 120);
    SDL_RenderDrawRectF(renderer, &ground);

    SDL_RenderSetClipRect(renderer, NULL);
}
