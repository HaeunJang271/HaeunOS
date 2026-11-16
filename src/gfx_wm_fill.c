#include "haeun/gfx_wm.h"

#include <stddef.h>
#include <stdio.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-wm] security/validation failure: %s\n", what);
}

/**
 * Fills [x,y,w,h) inside the window display rect (clipped; backing stride bw).
 */
haeun_gfx_result haeun_gfx_wm_fill_rect(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, uint32_t w, uint32_t h,
                                        uint32_t rgba) {
    haeun_gfx_window *win;
    uint32_t yy, xx, x1, y1;
    uint8_t b0, b1, b2, b3;
    if (wm == NULL || wm->vm == NULL || wm->vm->ram == NULL || id < 0 ||
        id >= HAEUN_GFX_WM_MAX_WINDOWS) {
        log_fail("wm_fill_rect bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    win = &wm->windows[id];
    if (!win->used) {
        log_fail("wm_fill_rect unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (w == 0u || h == 0u)
        return HAEUN_GFX_OK;
    if (x >= win->w || y >= win->h)
        return HAEUN_GFX_OOB;
    x1 = x + w;
    y1 = y + h;
    if (x1 < x || y1 < y)
        return HAEUN_GFX_BAD_ARGUMENT;
    if (x1 > win->w)
        x1 = win->w;
    if (y1 > win->h)
        y1 = win->h;
    b0 = (uint8_t)(rgba & 0xffu);
    b1 = (uint8_t)((rgba >> 8) & 0xffu);
    b2 = (uint8_t)((rgba >> 16) & 0xffu);
    b3 = (uint8_t)((rgba >> 24) & 0xffu);
    for (yy = y; yy < y1; ++yy) {
        for (xx = x; xx < x1; ++xx) {
            size_t off =
                (size_t)win->base +
                ((size_t)yy * (size_t)win->bw + (size_t)xx) * 4u;
            wm->vm->ram[off] = b0;
            wm->vm->ram[off + 1u] = b1;
            wm->vm->ram[off + 2u] = b2;
            wm->vm->ram[off + 3u] = b3;
        }
    }
    return HAEUN_GFX_OK;
}
