#include "haeun/gfx.h"

static void store_rgba(uint8_t *p, uint32_t rgba) {
    p[0] = (uint8_t)rgba;
    p[1] = (uint8_t)(rgba >> 8);
    p[2] = (uint8_t)(rgba >> 16);
    p[3] = (uint8_t)(rgba >> 24);
}

static uint32_t load_rgba(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint8_t *pixel_ptr(haeun_gfx *gfx, uint32_t x, uint32_t y) {
    uint64_t off =
        ((uint64_t)y * (uint64_t)gfx->width + (uint64_t)x) * 4u;
    return gfx->vm->ram + (size_t)gfx->base + (size_t)off;
}

/**
 * Stores one pixel after validating readiness and coordinates.
 */
haeun_gfx_result haeun_gfx_put_pixel(haeun_gfx *gfx, uint32_t x, uint32_t y,
                                     uint32_t rgba) {
    if (gfx == NULL || !haeun_gfx_fb_ready(gfx)) return HAEUN_GFX_NOT_READY;
    if (x >= gfx->width || y >= gfx->height) return HAEUN_GFX_OOB;
    store_rgba(pixel_ptr(gfx, x, y), rgba);
    return HAEUN_GFX_OK;
}

/**
 * Loads one pixel after validating readiness and coordinates.
 */
haeun_gfx_result haeun_gfx_get_pixel(const haeun_gfx *gfx, uint32_t x,
                                     uint32_t y, uint32_t *rgba) {
    const uint8_t *p;
    uint64_t off;
    if (gfx == NULL || rgba == NULL || !haeun_gfx_fb_ready(gfx))
        return HAEUN_GFX_NOT_READY;
    if (x >= gfx->width || y >= gfx->height) return HAEUN_GFX_OOB;
    off = ((uint64_t)y * (uint64_t)gfx->width + (uint64_t)x) * 4u;
    p = gfx->vm->ram + (size_t)gfx->base + (size_t)off;
    *rgba = load_rgba(p);
    return HAEUN_GFX_OK;
}

/**
 * Fills every pixel with rgba.
 */
haeun_gfx_result haeun_gfx_clear(haeun_gfx *gfx, uint32_t rgba) {
    return haeun_gfx_fill_rect(gfx, 0u, 0u, gfx != NULL ? gfx->width : 0u,
                               gfx != NULL ? gfx->height : 0u, rgba);
}

/**
 * Fills a rectangle, clipping to the framebuffer.
 */
haeun_gfx_result haeun_gfx_fill_rect(haeun_gfx *gfx, uint32_t x, uint32_t y,
                                     uint32_t w, uint32_t h, uint32_t rgba) {
    uint32_t x1, y1, xi, yi;
    if (gfx == NULL || !haeun_gfx_fb_ready(gfx)) return HAEUN_GFX_NOT_READY;
    if (w == 0u || h == 0u) return HAEUN_GFX_OK;
    if (x >= gfx->width || y >= gfx->height) return HAEUN_GFX_OOB;
    x1 = x + w;
    y1 = y + h;
    if (x1 < x || x1 > gfx->width) x1 = gfx->width;
    if (y1 < y || y1 > gfx->height) y1 = gfx->height;
    for (yi = y; yi < y1; ++yi) {
        for (xi = x; xi < x1; ++xi)
            store_rgba(pixel_ptr(gfx, xi, yi), rgba);
    }
    return HAEUN_GFX_OK;
}
