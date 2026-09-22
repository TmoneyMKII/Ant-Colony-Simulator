/**
 * @file font.c
 * @brief stb_truetype glyph atlas
 */

#include "font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#define FIRST_CHAR 32
#define CHAR_COUNT 95           /* printable ASCII, 32..126 */
#define ATLAS_SIZE 512
#define OVERSAMPLE 1            /* glyphs land on integer pixels, so 1 is sharpest */

struct Font {
    SDL_Texture *atlas;
    stbtt_packedchar glyphs[CHAR_COUNT];
    float pixel_height;
    int line_height;
    float ascent;               /**< Baseline offset from the top, in pixels */
};

static unsigned char *read_file(const char *path, long *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) {
        fclose(f);
        return NULL;
    }

    unsigned char *data = malloc((size_t)size);
    if (data && fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        data = NULL;
    }
    fclose(f);

    if (data) *out_size = size;
    return data;
}

Font *font_load(SDL_Renderer *renderer, const char *path, float pixel_height) {
    long ttf_size = 0;
    unsigned char *ttf = read_file(path, &ttf_size);
    if (!ttf) return NULL;

    Font *font = calloc(1, sizeof(Font));
    unsigned char *coverage = calloc(ATLAS_SIZE * ATLAS_SIZE, 1);
    if (!font || !coverage) {
        free(ttf);
        free(font);
        free(coverage);
        return NULL;
    }

    font->pixel_height = pixel_height;

    stbtt_pack_context pack;
    if (!stbtt_PackBegin(&pack, coverage, ATLAS_SIZE, ATLAS_SIZE, 0, 1, NULL)) {
        free(ttf);
        free(coverage);
        free(font);
        return NULL;
    }
    stbtt_PackSetOversampling(&pack, OVERSAMPLE, OVERSAMPLE);
    int packed = stbtt_PackFontRange(&pack, ttf, 0, pixel_height,
                                     FIRST_CHAR, CHAR_COUNT, font->glyphs);
    stbtt_PackEnd(&pack);

    /* Vertical metrics for the baseline and line spacing */
    stbtt_fontinfo info;
    if (packed && stbtt_InitFont(&info, ttf, stbtt_GetFontOffsetForIndex(ttf, 0))) {
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
        float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);
        font->ascent = (float)ascent * scale;
        font->line_height = (int)((float)(ascent - descent + line_gap) * scale + 0.5f);
    } else {
        font->ascent = pixel_height * 0.8f;
        font->line_height = (int)(pixel_height * 1.3f);
    }
    free(ttf);

    if (!packed) {
        free(coverage);
        free(font);
        return NULL;
    }

    /* Expand 8-bit coverage into white pixels with per-pixel alpha */
    uint32_t *pixels = malloc(sizeof(uint32_t) * ATLAS_SIZE * ATLAS_SIZE);
    if (!pixels) {
        free(coverage);
        free(font);
        return NULL;
    }
    for (int i = 0; i < ATLAS_SIZE * ATLAS_SIZE; i++) {
        pixels[i] = ((uint32_t)coverage[i] << 24) | 0x00FFFFFFu;  /* ARGB8888 */
    }
    free(coverage);

    font->atlas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STATIC, ATLAS_SIZE, ATLAS_SIZE);
    if (font->atlas) {
        SDL_UpdateTexture(font->atlas, NULL, pixels, ATLAS_SIZE * (int)sizeof(uint32_t));
        SDL_SetTextureBlendMode(font->atlas, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(font->atlas, SDL_ScaleModeNearest);
    }
    free(pixels);

    if (!font->atlas) {
        free(font);
        return NULL;
    }
    return font;
}

void font_destroy(Font *font) {
    if (!font) return;
    if (font->atlas) SDL_DestroyTexture(font->atlas);
    free(font);
}

int font_line_height(const Font *font) {
    return font ? font->line_height : 0;
}

static inline const stbtt_packedchar *glyph_for(const Font *font, unsigned char c) {
    if (c < FIRST_CHAR || c >= FIRST_CHAR + CHAR_COUNT) c = '?';
    return &font->glyphs[c - FIRST_CHAR];
}

int font_text_width(const Font *font, const char *text, int len) {
    if (!font || !text) return 0;
    if (len < 0) len = (int)strlen(text);

    float width = 0.0f;
    for (int i = 0; i < len; i++) {
        width += glyph_for(font, (unsigned char)text[i])->xadvance;
    }
    return (int)(width + 0.5f);
}

void font_draw(SDL_Renderer *renderer, Font *font, const char *text, int len,
               float x, float y, SDL_Color color) {
    if (!font || !text) return;
    if (len < 0) len = (int)strlen(text);

    SDL_SetTextureColorMod(font->atlas, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(font->atlas, color.a);

    float pen_x = x;
    float baseline = y + font->ascent;

    for (int i = 0; i < len; i++) {
        const stbtt_packedchar *g = glyph_for(font, (unsigned char)text[i]);

        SDL_Rect src = {g->x0, g->y0, g->x1 - g->x0, g->y1 - g->y0};
        if (src.w > 0 && src.h > 0) {
            /* Snap to whole pixels so the unfiltered atlas stays sharp */
            SDL_Rect dst = {(int)(pen_x + g->xoff + 0.5f), (int)(baseline + g->yoff + 0.5f),
                            src.w, src.h};
            SDL_RenderCopy(renderer, font->atlas, &src, &dst);
        }
        pen_x += g->xadvance;
    }
}
