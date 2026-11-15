#include "haeun/gfx_wm.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-wm] security/validation failure: %s\n", what);
}

static haeun_gfx_window *slot(haeun_gfx_wm *wm, int id) {
    if (wm == NULL || id < 0 || id >= HAEUN_GFX_WM_MAX_WINDOWS)
        return NULL;
    if (!wm->windows[id].used)
        return NULL;
    return &wm->windows[id];
}

/**
 * Clamps display size to the immutable backing; keeps content in-bounds.
 */
haeun_gfx_result haeun_gfx_wm_resize(haeun_gfx_wm *wm, int id, uint32_t w,
                                     uint32_t h) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL || w == 0u || h == 0u) {
        log_fail("wm_resize bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (w > win->bw)
        w = win->bw;
    if (h > win->bh)
        h = win->bh;
    win->w = w;
    win->h = h;
    win->maximized = 0;
    return HAEUN_GFX_OK;
}

/**
 * Stores a NUL-terminated title for host chrome (max TITLE_MAX-1).
 */
haeun_gfx_result haeun_gfx_wm_set_title(haeun_gfx_wm *wm, int id,
                                        const char *title) {
    haeun_gfx_window *win = slot(wm, id);
    size_t i;
    if (win == NULL) {
        log_fail("wm_set_title bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (title == NULL)
        title = "";
    for (i = 0; i + 1u < (size_t)HAEUN_GFX_WM_TITLE_MAX && title[i] != '\0';
         ++i)
        win->title[i] = title[i];
    win->title[i] = '\0';
    return HAEUN_GFX_OK;
}

/**
 * Minimizes a window and drops keyboard focus if it held it.
 */
haeun_gfx_result haeun_gfx_wm_minimize(haeun_gfx_wm *wm, int id) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_minimize bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    win->minimized = 1;
    win->visible = 0;
    if (wm->focus_id == id)
        wm->focus_id = -1;
    return HAEUN_GFX_OK;
}

/**
 * Maximizes display geometry within the framebuffer, clamped to backing.
 */
haeun_gfx_result haeun_gfx_wm_maximize(haeun_gfx_wm *wm, int id, uint32_t fb_w,
                                       uint32_t fb_h) {
    haeun_gfx_window *win = slot(wm, id);
    uint32_t nw;
    uint32_t nh;
    if (win == NULL || fb_w == 0u || fb_h == 0u) {
        log_fail("wm_maximize bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (!win->maximized) {
        win->rest_x = win->x;
        win->rest_y = win->y;
        win->rest_w = win->w;
        win->rest_h = win->h;
    }
    win->minimized = 0;
    win->visible = 1;
    win->maximized = 1;
    win->x = 0;
    win->y = 0;
    nw = fb_w < win->bw ? fb_w : win->bw;
    nh = fb_h < win->bh ? fb_h : win->bh;
    win->w = nw;
    win->h = nh;
    return haeun_gfx_wm_raise(wm, id);
}

/**
 * Restores pre-maximize geometry or un-minimizes.
 */
haeun_gfx_result haeun_gfx_wm_restore(haeun_gfx_wm *wm, int id) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_restore bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (win->minimized) {
        win->minimized = 0;
        win->visible = 1;
        return HAEUN_GFX_OK;
    }
    if (!win->maximized)
        return HAEUN_GFX_OK;
    win->maximized = 0;
    win->x = win->rest_x;
    win->y = win->rest_y;
    win->w = win->rest_w != 0u ? win->rest_w : win->bw;
    win->h = win->rest_h != 0u ? win->rest_h : win->bh;
    if (win->w > win->bw)
        win->w = win->bw;
    if (win->h > win->bh)
        win->h = win->bh;
    return HAEUN_GFX_OK;
}
