#ifndef HAEUN_GFX_H
#define HAEUN_GFX_H

#include "haeun/vm.h"

#include <stdint.h>

typedef enum {
    HAEUN_GFX_OK = 0,
    HAEUN_GFX_BAD_ARGUMENT,
    HAEUN_GFX_NOT_READY,
    HAEUN_GFX_OOB
} haeun_gfx_result;

typedef struct {
    haeun_vm *vm;
    uint32_t width;
    uint32_t height;
    uint32_t base;
    int ready;
} haeun_gfx;

/**
 * Binds gfx to vm and programs the framebuffer aperture.
 * base must be a guest PA inside RAM for width*height*4 bytes.
 */
haeun_gfx_result haeun_gfx_fb_enable(haeun_gfx *gfx, haeun_vm *vm,
                                     uint32_t width, uint32_t height,
                                     uint32_t base);

/** Disables the framebuffer CONTROL bit and clears ready. */
haeun_gfx_result haeun_gfx_fb_disable(haeun_gfx *gfx);

/** True when STATUS reports a valid enabled configuration. */
int haeun_gfx_fb_ready(const haeun_gfx *gfx);

/** Writes one RGBA8888 pixel; rejects out-of-range coordinates. */
haeun_gfx_result haeun_gfx_put_pixel(haeun_gfx *gfx, uint32_t x, uint32_t y,
                                     uint32_t rgba);

/** Reads one pixel; rejects out-of-range coordinates. */
haeun_gfx_result haeun_gfx_get_pixel(const haeun_gfx *gfx, uint32_t x,
                                     uint32_t y, uint32_t *rgba);

/** Fills the entire framebuffer with rgba. */
haeun_gfx_result haeun_gfx_clear(haeun_gfx *gfx, uint32_t rgba);

/** Fills a rectangle; clips to the framebuffer bounds. */
haeun_gfx_result haeun_gfx_fill_rect(haeun_gfx *gfx, uint32_t x, uint32_t y,
                                     uint32_t w, uint32_t h, uint32_t rgba);

/** Draws one 8×8 glyph; unsupported glyphs render as a solid block. */
haeun_gfx_result haeun_gfx_draw_char(haeun_gfx *gfx, uint32_t x, uint32_t y,
                                     char ch, uint32_t fg, uint32_t bg);

/** Draws a NUL-terminated string left-to-right, 8 px advance. */
haeun_gfx_result haeun_gfx_draw_string(haeun_gfx *gfx, uint32_t x, uint32_t y,
                                       const char *text, uint32_t fg,
                                       uint32_t bg);

/**
 * Returns 8 row bytes for a printable ASCII glyph, or space rows otherwise.
 * Bit7 of each row is the leftmost pixel.
 */
const uint8_t *haeun_gfx_font8_rows(char ch);

#endif

