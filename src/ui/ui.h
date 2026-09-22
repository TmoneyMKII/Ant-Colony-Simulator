/**
 * @file ui.h
 * @brief microui bound to SDL: theme, input, command rendering
 *
 * microui only knows rectangles, text and icons. Two custom commands are
 * added here (lines and discs) so panels can draw graphs and the network
 * diagram through the same clipped, ordered command list as everything else.
 */

#ifndef UI_H
#define UI_H

#include <SDL.h>
#include "microui.h"
#include "font.h"

/** @brief Command types beyond microui's own */
enum {
    UI_COMMAND_LINE = MU_COMMAND_MAX + 1,
    UI_COMMAND_DISC
};

typedef struct {
    mu_BaseCommand base;
    float x0, y0, x1, y1;
    float thickness;
    mu_Color color;
} UiLineCommand;

typedef struct {
    mu_BaseCommand base;
    float cx, cy;
    float radius;
    mu_Color color;
} UiDiscCommand;

/**
 * @brief Set up the context, theme and text callbacks
 * @param scale UI scale factor (fonts and metrics are already scaled by it)
 */
bool ui_init(Font *font, float scale);
void ui_shutdown(void);

mu_Context *ui_context(void);

/** @brief Feed an SDL event to the UI */
void ui_handle_event(const SDL_Event *event);

/** @brief True when the pointer is over a panel or dragging a control */
bool ui_wants_mouse(void);

/** @brief True when a text field has focus, so the app should ignore hotkeys */
bool ui_wants_keyboard(void);

void ui_begin_frame(void);
void ui_end_frame(void);

/** @brief Execute the command list */
void ui_render(SDL_Renderer *renderer);

/* ============== Extra draw commands ============== */

void ui_draw_line(mu_Context *ctx, float x0, float y0, float x1, float y1,
                  float thickness, mu_Color color);

void ui_draw_disc(mu_Context *ctx, float cx, float cy, float radius, mu_Color color);

#endif /* UI_H */
