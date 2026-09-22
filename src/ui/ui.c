/**
 * @file ui.c
 * @brief microui context, theme, input translation and SDL backend
 */

#include "ui.h"
#include <math.h>
#include <string.h>

#define DISC_SEGMENTS 20

static mu_Context g_ctx;
static Font *g_font = NULL;
static bool g_ready = false;

/* ============== Text metrics ============== */

static int text_width_cb(mu_Font font, const char *text, int len) {
    (void)font;
    return font_text_width(g_font, text, len);
}

static int text_height_cb(mu_Font font) {
    (void)font;
    return font_line_height(g_font);
}

/* ============== Theme ============== */

static void apply_theme(float scale) {
    mu_Style *s = g_ctx.style;

    s->colors[MU_COLOR_TEXT]        = mu_color(226, 228, 236, 255);
    s->colors[MU_COLOR_BORDER]      = mu_color(48, 50, 62, 255);
    s->colors[MU_COLOR_WINDOWBG]    = mu_color(22, 23, 30, 255);
    s->colors[MU_COLOR_TITLEBG]     = mu_color(30, 32, 42, 255);
    s->colors[MU_COLOR_TITLETEXT]   = mu_color(150, 210, 255, 255);
    s->colors[MU_COLOR_PANELBG]     = mu_color(17, 18, 24, 255);
    s->colors[MU_COLOR_BUTTON]      = mu_color(42, 45, 58, 255);
    s->colors[MU_COLOR_BUTTONHOVER] = mu_color(56, 62, 80, 255);
    s->colors[MU_COLOR_BUTTONFOCUS] = mu_color(70, 120, 160, 255);
    s->colors[MU_COLOR_BASE]        = mu_color(32, 34, 44, 255);
    s->colors[MU_COLOR_BASEHOVER]   = mu_color(40, 44, 56, 255);
    s->colors[MU_COLOR_BASEFOCUS]   = mu_color(52, 90, 120, 255);
    s->colors[MU_COLOR_SCROLLBASE]  = mu_color(28, 30, 38, 255);
    s->colors[MU_COLOR_SCROLLTHUMB] = mu_color(64, 70, 88, 255);

    int line = font_line_height(g_font);
    s->size.x = (int)(60 * scale);
    s->size.y = line;
    s->padding = (int)(6 * scale);
    s->spacing = (int)(5 * scale);
    s->indent = (int)(14 * scale);
    s->title_height = line + (int)(10 * scale);
    s->scrollbar_size = (int)(10 * scale);
    s->thumb_size = (int)(10 * scale);
}

bool ui_init(Font *font, float scale) {
    if (!font) return false;
    g_font = font;

    mu_init(&g_ctx);
    g_ctx.text_width = text_width_cb;
    g_ctx.text_height = text_height_cb;
    g_ctx.style->font = NULL;   /* one font, so the handle is unused */
    apply_theme(scale);

    g_ready = true;
    return true;
}

void ui_shutdown(void) {
    g_ready = false;
    g_font = NULL;
}

mu_Context *ui_context(void) {
    return &g_ctx;
}

/* ============== Input ============== */

static int mouse_button_bit(uint8_t sdl_button) {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT:   return MU_MOUSE_LEFT;
        case SDL_BUTTON_RIGHT:  return MU_MOUSE_RIGHT;
        case SDL_BUTTON_MIDDLE: return MU_MOUSE_MIDDLE;
        default:                return 0;
    }
}

static int key_bit(SDL_Keycode key) {
    switch (key) {
        case SDLK_LSHIFT: case SDLK_RSHIFT:  return MU_KEY_SHIFT;
        case SDLK_LCTRL:  case SDLK_RCTRL:   return MU_KEY_CTRL;
        case SDLK_LALT:   case SDLK_RALT:    return MU_KEY_ALT;
        case SDLK_BACKSPACE:                 return MU_KEY_BACKSPACE;
        case SDLK_RETURN: case SDLK_KP_ENTER: return MU_KEY_RETURN;
        default:                             return 0;
    }
}

void ui_handle_event(const SDL_Event *event) {
    if (!g_ready) return;

    switch (event->type) {
        case SDL_MOUSEMOTION:
            mu_input_mousemove(&g_ctx, event->motion.x, event->motion.y);
            break;

        case SDL_MOUSEBUTTONDOWN: {
            int btn = mouse_button_bit(event->button.button);
            if (btn) mu_input_mousedown(&g_ctx, event->button.x, event->button.y, btn);
            break;
        }

        case SDL_MOUSEBUTTONUP: {
            int btn = mouse_button_bit(event->button.button);
            if (btn) mu_input_mouseup(&g_ctx, event->button.x, event->button.y, btn);
            break;
        }

        case SDL_MOUSEWHEEL:
            mu_input_scroll(&g_ctx, 0, -event->wheel.y * 30);
            break;

        case SDL_TEXTINPUT:
            mu_input_text(&g_ctx, event->text.text);
            break;

        case SDL_KEYDOWN: {
            int bit = key_bit(event->key.keysym.sym);
            if (bit) mu_input_keydown(&g_ctx, bit);
            break;
        }

        case SDL_KEYUP: {
            int bit = key_bit(event->key.keysym.sym);
            if (bit) mu_input_keyup(&g_ctx, bit);
            break;
        }

        default:
            break;
    }
}

bool ui_wants_mouse(void) {
    /* hover_root reflects the panel under the pointer as of the last frame */
    return g_ready && (g_ctx.hover_root != NULL || g_ctx.focus != 0);
}

bool ui_wants_keyboard(void) {
    /* Only text fields keep focus once the mouse button is released */
    return g_ready && g_ctx.focus != 0 && g_ctx.mouse_down == 0;
}

void ui_begin_frame(void) {
    mu_begin(&g_ctx);
}

void ui_end_frame(void) {
    mu_end(&g_ctx);
}

/* ============== Custom commands ============== */

static const mu_Rect UNCLIPPED = {0, 0, 0x1000000, 0x1000000};

/** @brief Clip a custom command the way microui clips text and icons */
static int begin_clipped(mu_Context *ctx, mu_Rect bbox) {
    int clipped = mu_check_clip(ctx, bbox);
    if (clipped == MU_CLIP_PART) {
        mu_set_clip(ctx, mu_get_clip_rect(ctx));
    }
    return clipped;
}

static void end_clipped(mu_Context *ctx, int clipped) {
    if (clipped) {
        mu_set_clip(ctx, UNCLIPPED);
    }
}

void ui_draw_line(mu_Context *ctx, float x0, float y0, float x1, float y1,
                  float thickness, mu_Color color) {
    int pad = (int)thickness + 2;
    mu_Rect bbox = {
        (int)fminf(x0, x1) - pad,
        (int)fminf(y0, y1) - pad,
        (int)fabsf(x1 - x0) + 2 * pad,
        (int)fabsf(y1 - y0) + 2 * pad
    };

    int clipped = begin_clipped(ctx, bbox);
    if (clipped == MU_CLIP_ALL) return;

    UiLineCommand *cmd = (UiLineCommand *)mu_push_command(ctx, UI_COMMAND_LINE,
                                                          sizeof(UiLineCommand));
    cmd->x0 = x0;
    cmd->y0 = y0;
    cmd->x1 = x1;
    cmd->y1 = y1;
    cmd->thickness = thickness;
    cmd->color = color;

    end_clipped(ctx, clipped);
}

void ui_draw_disc(mu_Context *ctx, float cx, float cy, float radius, mu_Color color) {
    mu_Rect bbox = {(int)(cx - radius) - 1, (int)(cy - radius) - 1,
                    (int)(radius * 2.0f) + 2, (int)(radius * 2.0f) + 2};

    int clipped = begin_clipped(ctx, bbox);
    if (clipped == MU_CLIP_ALL) return;

    UiDiscCommand *cmd = (UiDiscCommand *)mu_push_command(ctx, UI_COMMAND_DISC,
                                                          sizeof(UiDiscCommand));
    cmd->cx = cx;
    cmd->cy = cy;
    cmd->radius = radius;
    cmd->color = color;

    end_clipped(ctx, clipped);
}

/* ============== SDL backend ============== */

static inline void set_draw_color(SDL_Renderer *renderer, mu_Color c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

static void fill_triangles(SDL_Renderer *renderer, const SDL_Vertex *verts, int count) {
    SDL_RenderGeometry(renderer, NULL, verts, count, NULL, 0);
}

static void draw_thick_line(SDL_Renderer *renderer, const UiLineCommand *line) {
    if (line->thickness <= 1.4f) {
        set_draw_color(renderer, line->color);
        SDL_RenderDrawLineF(renderer, line->x0, line->y0, line->x1, line->y1);
        return;
    }

    /* Wide lines become a quad so the thickness is honoured */
    float dx = line->x1 - line->x0;
    float dy = line->y1 - line->y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float nx = -dy / len * line->thickness * 0.5f;
    float ny = dx / len * line->thickness * 0.5f;
    SDL_Color color = {line->color.r, line->color.g, line->color.b, line->color.a};

    SDL_Vertex v[6];
    const float xs[6] = {line->x0 + nx, line->x1 + nx, line->x1 - nx,
                         line->x0 + nx, line->x1 - nx, line->x0 - nx};
    const float ys[6] = {line->y0 + ny, line->y1 + ny, line->y1 - ny,
                         line->y0 + ny, line->y1 - ny, line->y0 - ny};
    for (int i = 0; i < 6; i++) {
        v[i].position.x = xs[i];
        v[i].position.y = ys[i];
        v[i].color = color;
        v[i].tex_coord.x = 0.0f;
        v[i].tex_coord.y = 0.0f;
    }
    fill_triangles(renderer, v, 6);
}

static void draw_disc(SDL_Renderer *renderer, const UiDiscCommand *disc) {
    SDL_Color color = {disc->color.r, disc->color.g, disc->color.b, disc->color.a};
    SDL_Vertex v[DISC_SEGMENTS * 3];

    for (int i = 0; i < DISC_SEGMENTS; i++) {
        float a0 = 6.28318531f * (float)i / DISC_SEGMENTS;
        float a1 = 6.28318531f * (float)(i + 1) / DISC_SEGMENTS;
        SDL_Vertex *t = &v[i * 3];

        t[0].position.x = disc->cx;
        t[0].position.y = disc->cy;
        t[1].position.x = disc->cx + cosf(a0) * disc->radius;
        t[1].position.y = disc->cy + sinf(a0) * disc->radius;
        t[2].position.x = disc->cx + cosf(a1) * disc->radius;
        t[2].position.y = disc->cy + sinf(a1) * disc->radius;

        for (int k = 0; k < 3; k++) {
            t[k].color = color;
            t[k].tex_coord.x = 0.0f;
            t[k].tex_coord.y = 0.0f;
        }
    }
    fill_triangles(renderer, v, DISC_SEGMENTS * 3);
}

/** @brief microui's four built-in icons, drawn from primitives */
static void draw_icon(SDL_Renderer *renderer, const mu_IconCommand *icon) {
    mu_Rect r = icon->rect;
    float cx = (float)r.x + (float)r.w * 0.5f;
    float cy = (float)r.y + (float)r.h * 0.5f;
    float s = (float)((r.w < r.h ? r.w : r.h)) * 0.28f;
    SDL_Color color = {icon->color.r, icon->color.g, icon->color.b, icon->color.a};
    set_draw_color(renderer, icon->color);

    switch (icon->id) {
        case MU_ICON_CLOSE:
            SDL_RenderDrawLineF(renderer, cx - s, cy - s, cx + s, cy + s);
            SDL_RenderDrawLineF(renderer, cx + s, cy - s, cx - s, cy + s);
            break;

        case MU_ICON_CHECK:
            SDL_RenderDrawLineF(renderer, cx - s, cy, cx - s * 0.2f, cy + s * 0.8f);
            SDL_RenderDrawLineF(renderer, cx - s * 0.2f, cy + s * 0.8f, cx + s, cy - s * 0.7f);
            SDL_RenderDrawLineF(renderer, cx - s, cy + 1, cx - s * 0.2f, cy + s * 0.8f + 1);
            SDL_RenderDrawLineF(renderer, cx - s * 0.2f, cy + s * 0.8f + 1, cx + s, cy - s * 0.7f + 1);
            break;

        case MU_ICON_COLLAPSED:
        case MU_ICON_EXPANDED: {
            SDL_Vertex v[3];
            if (icon->id == MU_ICON_COLLAPSED) {
                v[0].position = (SDL_FPoint){cx - s * 0.6f, cy - s};
                v[1].position = (SDL_FPoint){cx - s * 0.6f, cy + s};
                v[2].position = (SDL_FPoint){cx + s * 0.8f, cy};
            } else {
                v[0].position = (SDL_FPoint){cx - s, cy - s * 0.6f};
                v[1].position = (SDL_FPoint){cx + s, cy - s * 0.6f};
                v[2].position = (SDL_FPoint){cx, cy + s * 0.8f};
            }
            for (int i = 0; i < 3; i++) {
                v[i].color = color;
                v[i].tex_coord.x = 0.0f;
                v[i].tex_coord.y = 0.0f;
            }
            fill_triangles(renderer, v, 3);
            break;
        }

        default:
            break;
    }
}

void ui_render(SDL_Renderer *renderer) {
    if (!g_ready) return;

    mu_Command *cmd = NULL;
    while (mu_next_command(&g_ctx, &cmd)) {
        switch (cmd->type) {
            case MU_COMMAND_CLIP: {
                mu_Rect r = cmd->clip.rect;
                if (r.w >= 0x1000000) {
                    SDL_RenderSetClipRect(renderer, NULL);
                } else {
                    SDL_Rect clip = {r.x, r.y, r.w, r.h};
                    SDL_RenderSetClipRect(renderer, &clip);
                }
                break;
            }

            case MU_COMMAND_RECT: {
                set_draw_color(renderer, cmd->rect.color);
                SDL_Rect r = {cmd->rect.rect.x, cmd->rect.rect.y,
                              cmd->rect.rect.w, cmd->rect.rect.h};
                SDL_RenderFillRect(renderer, &r);
                break;
            }

            case MU_COMMAND_TEXT: {
                SDL_Color color = {cmd->text.color.r, cmd->text.color.g,
                                   cmd->text.color.b, cmd->text.color.a};
                font_draw(renderer, g_font, cmd->text.str, -1,
                          (float)cmd->text.pos.x, (float)cmd->text.pos.y, color);
                break;
            }

            case MU_COMMAND_ICON:
                draw_icon(renderer, &cmd->icon);
                break;

            case UI_COMMAND_LINE:
                draw_thick_line(renderer, (const UiLineCommand *)cmd);
                break;

            case UI_COMMAND_DISC:
                draw_disc(renderer, (const UiDiscCommand *)cmd);
                break;

            default:
                break;
        }
    }

    SDL_RenderSetClipRect(renderer, NULL);
}
