/**
 * @file font.h
 * @brief TrueType text rendering through a baked glyph atlas
 *
 * One atlas texture holds printable ASCII at a fixed pixel size. Glyphs
 * are drawn as textured quads, which SDL batches into few draw calls.
 */

#ifndef FONT_H
#define FONT_H

#include <SDL.h>
#include <stdbool.h>

typedef struct Font Font;

/**
 * @brief Bake a font file at the given pixel height
 * @return NULL if the file is missing or the atlas cannot be created
 */
Font *font_load(SDL_Renderer *renderer, const char *path, float pixel_height);

void font_destroy(Font *font);

/** @brief Distance between baselines, in pixels */
int font_line_height(const Font *font);

/** @brief Pixel width of text (len < 0 means NUL-terminated) */
int font_text_width(const Font *font, const char *text, int len);

/** @brief Draw text with its top-left corner at (x, y) */
void font_draw(SDL_Renderer *renderer, Font *font, const char *text, int len,
               float x, float y, SDL_Color color);

#endif /* FONT_H */
