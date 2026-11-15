#include "haeun/gfx_wm.h"

#include <stdio.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-comp] security/validation failure: %s\n", what);
}

static uint32_t load_rgba(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int order_windows(const haeun_gfx_wm *wm, int *order, int *count) {
    int n = 0;
    int i, j;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        if (wm->windows[i].used && wm->windows[i].visible)
            order[n++] = i;
    }
    for (i = 1; i < n; ++i) {
        int key = order[i];
        j = i - 1;
        while (j >= 0 && wm->windows[order[j]].z > wm->windows[key].z) {
            order[j + 1] = order[j];
            --j;
        }
        order[j + 1] = key;
    }
    *count = n;
    return 0;
}

static haeun_gfx_result blit_window(haeun_gfx *gfx, const haeun_gfx_wm *wm,
                                    const haeun_gfx_window *win) {
    uint32_t wy, wx;
    for (wy = 0u; wy < win->h; ++wy) {
        for (wx = 0u; wx < win->w; ++wx) {
            int32_t sx = win->x + (int32_t)wx;
            int32_t sy = win->y + (int32_t)wy;
            uint64_t off;
            uint32_t rgba;
            haeun_gfx_result r;
            if (sx < 0 || sy < 0) continue;
            if ((uint32_t)sx >= gfx->width || (uint32_t)sy >= gfx->height)
                continue;
            off = ((uint64_t)wy * (uint64_t)win->bw + (uint64_t)wx) * 4u;
            rgba = load_rgba(wm->vm->ram + (size_t)win->base + (size_t)off);
            r = haeun_gfx_put_pixel(gfx, (uint32_t)sx, (uint32_t)sy, rgba);
            if (r != HAEUN_GFX_OK && r != HAEUN_GFX_OOB) return r;
        }
    }
    return HAEUN_GFX_OK;
}

/**
 * Clears the primary FB then composites visible windows by ascending z.
 */
haeun_gfx_result haeun_gfx_compose(haeun_gfx *gfx, haeun_gfx_wm *wm,
                                   uint32_t desktop_rgba) {
    int order[HAEUN_GFX_WM_MAX_WINDOWS];
    int count = 0;
    int i;
    haeun_gfx_result r;
    if (gfx == NULL || wm == NULL || wm->vm == NULL) {
        log_fail("compose bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (wm->vm != gfx->vm) {
        log_fail("compose wm/gfx vm mismatch");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (!haeun_gfx_fb_ready(gfx)) {
        log_fail("compose framebuffer not ready");
        return HAEUN_GFX_NOT_READY;
    }
    r = haeun_gfx_clear(gfx, desktop_rgba);
    if (r != HAEUN_GFX_OK) return r;
    order_windows(wm, order, &count);
    for (i = 0; i < count; ++i) {
        const haeun_gfx_window *win = &wm->windows[order[i]];
        uint64_t end =
            (uint64_t)win->base + (uint64_t)win->w * (uint64_t)win->h * 4u;
        if (end > (uint64_t)wm->vm->ram_size) {
            log_fail("compose backing escaped RAM");
            return HAEUN_GFX_BAD_ARGUMENT;
        }
        r = blit_window(gfx, wm, win);
        if (r != HAEUN_GFX_OK) return r;
    }
    return HAEUN_GFX_OK;
}
