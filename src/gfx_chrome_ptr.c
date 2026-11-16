#include "haeun/gfx_chrome.h"

enum {
    ZONE_NONE = 0,
    ZONE_CLIENT,
    ZONE_TITLE,
    ZONE_MIN,
    ZONE_MAX,
    ZONE_CLOSE,
    ZONE_RESIZE
};

/** True for the desktop shell surface (covers the primary FB). */
static int is_shell(const haeun_gfx_chrome *ch, const haeun_gfx_window *win) {
    return ch != NULL && ch->gfx != NULL && win != NULL && win->used &&
           win->w >= ch->gfx->width && win->h >= ch->gfx->height;
}

static int zone_at(const haeun_gfx_window *win, int32_t sx, int32_t sy) {
    int32_t tx = win->x;
    int32_t ty = win->y - (int32_t)HAEUN_CHROME_TITLE_H;
    int32_t tw = (int32_t)win->w;
    int32_t btn_x;
    if (win->minimized || !win->visible)
        return ZONE_NONE;
    if (sx >= win->x + (int32_t)win->w - 3 &&
        sy >= win->y + (int32_t)win->h - 3 &&
        sx < win->x + (int32_t)win->w + (int32_t)HAEUN_CHROME_BORDER &&
        sy < win->y + (int32_t)win->h + (int32_t)HAEUN_CHROME_BORDER)
        return ZONE_RESIZE;
    if (sx >= win->x && sy >= win->y && sx < win->x + (int32_t)win->w &&
        sy < win->y + (int32_t)win->h)
        return ZONE_CLIENT;
    if (sy < ty || sy >= win->y || sx < tx || sx >= tx + tw)
        return ZONE_NONE;
    btn_x = tx + tw - (int32_t)HAEUN_CHROME_BTN;
    if (sx >= btn_x)
        return ZONE_CLOSE;
    btn_x -= (int32_t)HAEUN_CHROME_BTN;
    if (sx >= btn_x)
        return ZONE_MAX;
    btn_x -= (int32_t)HAEUN_CHROME_BTN;
    if (sx >= btn_x)
        return ZONE_MIN;
    return ZONE_TITLE;
}

/**
 * Topmost chrome zone at (sx,sy). Non-shell app windows beat shell surfaces
 * so a raised desktop root cannot steal clicks from lower-z floating apps.
 */
static int top_zone(haeun_gfx_chrome *ch, int32_t sx, int32_t sy, int *out_id,
                    int *out_zone) {
    int i, best = -1, best_zone = ZONE_NONE, best_shell = 0;
    uint32_t best_z = 0u;
    haeun_gfx_wm *wm = ch->wm;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        haeun_gfx_window *w = &wm->windows[i];
        int z;
        int shell;
        if (!w->used || !w->visible || w->minimized)
            continue;
        z = zone_at(w, sx, sy);
        if (z == ZONE_NONE)
            continue;
        shell = is_shell(ch, w);
        if (best < 0) {
            best = i;
            best_z = w->z;
            best_zone = z;
            best_shell = shell;
            continue;
        }
        if (best_shell && !shell) {
            best = i;
            best_z = w->z;
            best_zone = z;
            best_shell = 0;
            continue;
        }
        if (!best_shell && shell)
            continue;
        if (w->z >= best_z) {
            best = i;
            best_z = w->z;
            best_zone = z;
            best_shell = shell;
        }
    }
    *out_id = best;
    *out_zone = best_zone;
    return best >= 0 ? 0 : -1;
}

/** Mouse-driven focus, drag, resize, min/max/close. */
haeun_gfx_result haeun_gfx_chrome_pointer(haeun_gfx_chrome *ch, int32_t sx,
                                          int32_t sy, uint32_t buttons) {
    uint32_t prev;
    int id = -1, zone = ZONE_NONE, down, rising;
    if (ch == NULL || !ch->attached)
        return HAEUN_GFX_BAD_ARGUMENT;
    prev = ch->prev_buttons;
    ch->prev_buttons = buttons & 7u;
    down = (buttons & 1u) != 0u;
    rising = down && (prev & 1u) == 0u;
    if (ch->drag_id >= 0) {
        if (!down) {
            haeun_gfx_window *dw = &ch->wm->windows[ch->drag_id];
            if (dw->used && ch->evt != NULL &&
                (dw->x != ch->drag_ox || dw->y != ch->drag_oy))
                haeun_evt_win_post(ch->evt, ch->drag_id, HAEUN_EVT_MOVED,
                                   (uint32_t)dw->x, (uint32_t)dw->y);
            ch->drag_id = -1;
        } else if (!is_shell(ch, &ch->wm->windows[ch->drag_id])) {
            (void)haeun_gfx_wm_move(ch->wm, ch->drag_id, sx - ch->drag_dx,
                                    sy - ch->drag_dy);
            ch->dirty = 1;
        } else {
            ch->drag_id = -1;
        }
        return HAEUN_GFX_OK;
    }
    if (ch->resize_id >= 0) {
        if (!down) {
            haeun_gfx_window *rw = &ch->wm->windows[ch->resize_id];
            if (rw->used && ch->evt != NULL &&
                (rw->w != ch->resize_ow || rw->h != ch->resize_oh))
                haeun_evt_win_post(ch->evt, ch->resize_id, HAEUN_EVT_RESIZE,
                                   rw->w, rw->h);
            ch->resize_id = -1;
        } else if (!is_shell(ch, &ch->wm->windows[ch->resize_id])) {
            int32_t nw = (int32_t)ch->resize_ow + (sx - ch->resize_ox);
            int32_t nh = (int32_t)ch->resize_oh + (sy - ch->resize_oy);
            if (nw < 4)
                nw = 4;
            if (nh < 4)
                nh = 4;
            (void)haeun_gfx_wm_resize(ch->wm, ch->resize_id, (uint32_t)nw,
                                      (uint32_t)nh);
            ch->dirty = 1;
        } else {
            ch->resize_id = -1;
        }
        return HAEUN_GFX_OK;
    }
    if (!rising) {
        if (ch->evt != NULL)
            haeun_evt_route_mouse(ch->evt, sx, sy, buttons, prev);
        return HAEUN_GFX_OK;
    }
    if (top_zone(ch, sx, sy, &id, &zone) != 0) {
        (void)haeun_gfx_wm_set_focus(ch->wm, -1);
        if (ch->evt != NULL)
            haeun_evt_on_focus(ch->evt, -1);
        ch->dirty = 1;
        return HAEUN_GFX_OK;
    }
    (void)haeun_gfx_wm_raise(ch->wm, id);
    (void)haeun_gfx_wm_set_focus(ch->wm, id);
    if (ch->evt != NULL)
        haeun_evt_on_focus(ch->evt, id);
    ch->dirty = 1;
    if (zone == ZONE_CLOSE) {
        int opid = ch->wm->windows[id].owner_pid;
        if (ch->evt != NULL)
            haeun_evt_win_post(ch->evt, id, HAEUN_EVT_CLOSE, 0u, 0u);
        if (opid <= 0) {
            (void)haeun_gfx_wm_destroy(ch->wm, id);
            if (ch->evt != NULL)
                haeun_evt_sync_focus(ch->evt);
        }
        return HAEUN_GFX_OK;
    }
    if (zone == ZONE_MIN) {
        if (haeun_gfx_wm_minimize(ch->wm, id) == HAEUN_GFX_OK && ch->evt != NULL)
            haeun_evt_win_post(ch->evt, id, HAEUN_EVT_MINIMIZED, 0u, 0u);
        if (ch->evt != NULL)
            haeun_evt_sync_focus(ch->evt);
        return HAEUN_GFX_OK;
    }
    if (zone == ZONE_MAX) {
        haeun_gfx_window *w = &ch->wm->windows[id];
        if (w->maximized) {
            if (haeun_gfx_wm_restore(ch->wm, id) == HAEUN_GFX_OK &&
                ch->evt != NULL)
                haeun_evt_win_post(ch->evt, id, HAEUN_EVT_RESTORED, w->w, w->h);
        } else if (haeun_gfx_wm_maximize(ch->wm, id, ch->gfx->width,
                                         ch->gfx->height) == HAEUN_GFX_OK &&
                   ch->evt != NULL) {
            haeun_evt_win_post(ch->evt, id, HAEUN_EVT_MAXIMIZED,
                               ch->wm->windows[id].w, ch->wm->windows[id].h);
        }
        return HAEUN_GFX_OK;
    }
    if (is_shell(ch, &ch->wm->windows[id])) {
        if (ch->evt != NULL && zone == ZONE_CLIENT)
            haeun_evt_route_mouse(ch->evt, sx, sy, buttons, prev);
        return HAEUN_GFX_OK;
    }
    if (zone == ZONE_TITLE) {
        haeun_gfx_window *w = &ch->wm->windows[id];
        ch->drag_id = id;
        ch->drag_dx = sx - w->x;
        ch->drag_dy = sy - w->y;
        ch->drag_ox = w->x;
        ch->drag_oy = w->y;
        return HAEUN_GFX_OK;
    }
    if (zone == ZONE_RESIZE) {
        haeun_gfx_window *w = &ch->wm->windows[id];
        ch->resize_id = id;
        ch->resize_ox = sx;
        ch->resize_oy = sy;
        ch->resize_ow = w->w;
        ch->resize_oh = w->h;
    } else if (ch->evt != NULL && zone == ZONE_CLIENT) {
        haeun_evt_route_mouse(ch->evt, sx, sy, buttons, prev);
    }
    return HAEUN_GFX_OK;
}
