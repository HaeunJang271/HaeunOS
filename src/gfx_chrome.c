#include "haeun/gfx_chrome.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-chrome] security/validation failure: %s\n", what);
}

static void fill_rect(haeun_gfx *gfx, int32_t x, int32_t y, uint32_t w,
                      uint32_t h, uint32_t rgba) {
    uint32_t yy, xx;
    for (yy = 0u; yy < h; ++yy)
        for (xx = 0u; xx < w; ++xx) {
            int32_t sx = x + (int32_t)xx, sy = y + (int32_t)yy;
            if (sx >= 0 && sy >= 0)
                (void)haeun_gfx_put_pixel(gfx, (uint32_t)sx, (uint32_t)sy, rgba);
        }
}

/** True for the desktop shell surface (covers the primary FB). */
static int is_shell(const haeun_gfx_chrome *ch, const haeun_gfx_window *win) {
    return ch != NULL && ch->gfx != NULL && win != NULL && win->used &&
           win->w >= ch->gfx->width && win->h >= ch->gfx->height;
}

/** Attaches chrome gesture state to gfx/wm. */
haeun_gfx_result haeun_gfx_chrome_init(haeun_gfx_chrome *ch, haeun_gfx *gfx,
                                       haeun_gfx_wm *wm) {
    if (ch == NULL || gfx == NULL || wm == NULL || !haeun_gfx_fb_ready(gfx)) {
        log_fail("chrome_init bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    memset(ch, 0, sizeof(*ch));
    ch->gfx = gfx;
    ch->wm = wm;
    ch->drag_id = -1;
    ch->resize_id = -1;
    ch->attached = 1;
    return HAEUN_GFX_OK;
}

static void paint_one(haeun_gfx_chrome *ch, int id) {
    haeun_gfx_window *win = &ch->wm->windows[id];
    int32_t ty = win->y - (int32_t)HAEUN_CHROME_TITLE_H;
    uint32_t title_bg = ch->wm->focus_id == id ? 0x3366AAFFu : 0x555555FFu;
    int32_t bx;
    if (win->minimized || !win->visible || is_shell(ch, win))
        return; /* no host chrome on fullscreen shell */
    fill_rect(ch->gfx, win->x, ty, win->w, HAEUN_CHROME_TITLE_H, title_bg);
    fill_rect(ch->gfx, win->x - 1, ty, 1u,
              HAEUN_CHROME_TITLE_H + win->h, 0x222222FFu);
    fill_rect(ch->gfx, win->x + (int32_t)win->w, ty, 1u,
              HAEUN_CHROME_TITLE_H + win->h, 0x222222FFu);
    fill_rect(ch->gfx, win->x, win->y + (int32_t)win->h, win->w, 1u,
              0x222222FFu);
    bx = win->x + (int32_t)win->w - (int32_t)HAEUN_CHROME_BTN;
    fill_rect(ch->gfx, bx, ty + 1, HAEUN_CHROME_BTN - 1u, 4u, 0xCC3333FFu);
    bx -= (int32_t)HAEUN_CHROME_BTN;
    fill_rect(ch->gfx, bx, ty + 1, HAEUN_CHROME_BTN - 1u, 4u, 0xCCCC33FFu);
    bx -= (int32_t)HAEUN_CHROME_BTN;
    fill_rect(ch->gfx, bx, ty + 1, HAEUN_CHROME_BTN - 1u, 4u, 0x33CC33FFu);
    fill_rect(ch->gfx, win->x + (int32_t)win->w - 2,
              win->y + (int32_t)win->h - 2, 3u, 3u, 0xFFFFFFFFu);
}

/** Full compose plus per-window decorations on the primary FB. */
haeun_gfx_result haeun_gfx_chrome_present(haeun_gfx_chrome *ch,
                                          uint32_t desktop_rgba) {
    int i;
    haeun_gfx_result r;
    if (ch == NULL || !ch->attached)
        return HAEUN_GFX_BAD_ARGUMENT;
    /* Keep shell pinned fullscreen so it never looks like a floating window. */
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        haeun_gfx_window *w = &ch->wm->windows[i];
        if (is_shell(ch, w) && (w->x != 0 || w->y != 0))
            (void)haeun_gfx_wm_move(ch->wm, i, 0, 0);
    }
    r = haeun_gfx_compose(ch->gfx, ch->wm, desktop_rgba);
    if (r != HAEUN_GFX_OK)
        return r;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        if (ch->wm->windows[i].used)
            paint_one(ch, i);
    }
    ch->dirty = 0;
    return HAEUN_GFX_OK;
}

/** Routes a scancode to the focused window bookkeeping slot. */
haeun_gfx_result haeun_gfx_chrome_key(haeun_gfx_chrome *ch, uint8_t scancode) {
    int is_break;
    int cur;
    if (ch == NULL || !ch->attached)
        return HAEUN_GFX_BAD_ARGUMENT;
    cur = ch->wm->focus_id;
    /* Adopt topmost when no focus or focus points at min/hidden/destroyed. */
    if (cur < 0 || cur >= HAEUN_GFX_WM_MAX_WINDOWS ||
        !ch->wm->windows[cur].used || !ch->wm->windows[cur].visible ||
        ch->wm->windows[cur].minimized) {
        (void)haeun_gfx_wm_focus_recover(ch->wm);
        if (ch->wm->focus_id >= 0 && ch->evt != NULL)
            haeun_evt_on_focus(ch->evt, ch->wm->focus_id);
    }
    if (ch->wm->focus_id < 0)
        return HAEUN_GFX_OK;
    ch->last_key = scancode;
    is_break = (scancode & 0x80u) != 0u;
    if (ch->evt != NULL)
        haeun_evt_route_key(ch->evt, (uint8_t)(scancode & 0x7fu), is_break);
    return HAEUN_GFX_OK;
}
