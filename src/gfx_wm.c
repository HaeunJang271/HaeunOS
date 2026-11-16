#include "haeun/gfx_wm.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-wm] security/validation failure: %s\n", what);
}

static int ranges_overlap(uint32_t a0, uint64_t a1, uint32_t b0, uint64_t b1) {
    return a0 < b1 && b0 < a1;
}

static int backing_ok(const haeun_vm *vm, uint32_t base, uint32_t w,
                      uint32_t h) {
    uint64_t bytes;
    uint64_t end;
    if (vm == NULL || w == 0u || h == 0u || w > 1024u || h > 1024u)
        return 0;
    if ((base & 3u) != 0u)
        return 0;
    bytes = (uint64_t)w * (uint64_t)h * 4u;
    end = (uint64_t)base + bytes;
    return end >= (uint64_t)base && end <= (uint64_t)vm->ram_size;
}

/**
 * True when base..end lies entirely in the guest window pool.
 */
int haeun_gfx_wm_in_user_pool(uint32_t base, uint32_t w, uint32_t h) {
    uint64_t bytes;
    uint64_t end;
    if (w == 0u || h == 0u || w > 1024u || h > 1024u)
        return 0;
    if ((base & 3u) != 0u)
        return 0;
    bytes = (uint64_t)w * (uint64_t)h * 4u;
    end = (uint64_t)base + bytes;
    if (end < (uint64_t)base)
        return 0;
    return base >= HAEUN_WIN_POOL_LO && end <= (uint64_t)HAEUN_WIN_POOL_HI;
}

/**
 * Validates alignment, RAM bounds, FB exclusion, and no window overlap.
 */
int haeun_gfx_wm_backing_allowed(const haeun_gfx_wm *wm, const haeun_gfx *gfx,
                                 uint32_t base, uint32_t w, uint32_t h) {
    uint64_t bytes;
    uint64_t end;
    int i;
    if (wm == NULL || wm->vm == NULL || !backing_ok(wm->vm, base, w, h))
        return 0;
    bytes = (uint64_t)w * (uint64_t)h * 4u;
    end = (uint64_t)base + bytes;
    if (gfx != NULL && gfx->ready) {
        uint64_t fb_bytes = (uint64_t)gfx->width * (uint64_t)gfx->height * 4u;
        uint64_t fb_end = (uint64_t)gfx->base + fb_bytes;
        if (ranges_overlap(base, end, gfx->base, fb_end))
            return 0;
    }
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        const haeun_gfx_window *win = &wm->windows[i];
        uint64_t other_bytes;
        uint64_t other_end;
        if (!win->used)
            continue;
        other_bytes = (uint64_t)win->bw * (uint64_t)win->bh * 4u;
        other_end = (uint64_t)win->base + other_bytes;
        if (ranges_overlap(base, end, win->base, other_end))
            return 0;
    }
    return 1;
}

static haeun_gfx_window *slot(haeun_gfx_wm *wm, int id) {
    if (wm == NULL || id < 0 || id >= HAEUN_GFX_WM_MAX_WINDOWS)
        return NULL;
    if (!wm->windows[id].used) return NULL;
    return &wm->windows[id];
}

static void store_rgba(uint8_t *p, uint32_t rgba) {
    p[0] = (uint8_t)rgba;
    p[1] = (uint8_t)(rgba >> 8);
    p[2] = (uint8_t)(rgba >> 16);
    p[3] = (uint8_t)(rgba >> 24);
}

/**
 * Initializes an empty window manager bound to vm.
 */
haeun_gfx_result haeun_gfx_wm_init(haeun_gfx_wm *wm, haeun_vm *vm) {
    if (wm == NULL || vm == NULL) {
        log_fail("wm_init bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    memset(wm, 0, sizeof(*wm));
    wm->vm = vm;
    wm->focus_id = -1;
    return HAEUN_GFX_OK;
}

/**
 * Creates a window whose backing lies entirely in guest RAM.
 */
haeun_gfx_result haeun_gfx_wm_create(haeun_gfx_wm *wm, int32_t x, int32_t y,
                                     uint32_t w, uint32_t h, uint32_t base,
                                     int *out_id) {
    int i;
    if (wm == NULL || wm->vm == NULL || out_id == NULL) {
        log_fail("wm_create bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (!backing_ok(wm->vm, base, w, h)) {
        log_fail("wm_create backing outside RAM or unaligned");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (!haeun_gfx_wm_backing_allowed(wm, NULL, base, w, h)) {
        log_fail("wm_create backing overlaps another window");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        if (!wm->windows[i].used) {
            haeun_gfx_window *win = &wm->windows[i];
            memset(win, 0, sizeof(*win));
            win->used = 1;
            win->visible = 1;
            win->minimized = 0;
            win->maximized = 0;
            win->x = x;
            win->y = y;
            win->w = w;
            win->h = h;
            win->bw = w;
            win->bh = h;
            win->base = base;
            win->z = wm->next_z++;
            win->owner_pid = 0;
            win->title[0] = '\0';
            *out_id = i;
            return HAEUN_GFX_OK;
        }
    }
    log_fail("wm_create table full");
    return HAEUN_GFX_BAD_ARGUMENT;
}

/** Sets the owning process pid (0 clears ownership). */
haeun_gfx_result haeun_gfx_wm_set_owner(haeun_gfx_wm *wm, int id, int pid) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_set_owner unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    win->owner_pid = pid;
    return HAEUN_GFX_OK;
}

/**
 * Returns 1 when the window is used and owned by pid (exact match).
 */
int haeun_gfx_wm_owned_by(const haeun_gfx_wm *wm, int id, int pid) {
    const haeun_gfx_window *win;
    if (wm == NULL || id < 0 || id >= HAEUN_GFX_WM_MAX_WINDOWS || pid == 0)
        return 0;
    win = &wm->windows[id];
    if (!win->used)
        return 0;
    return win->owner_pid == pid;
}

/**
 * Returns the topmost used+visible+!minimized window id by z-order, or -1.
 * Prefers guest-owned windows (owner_pid > 0) over unowned shell surfaces.
 */
int haeun_gfx_wm_top_eligible(const haeun_gfx_wm *wm) {
    int i;
    int best_any = -1;
    int best_owned = -1;
    uint32_t best_any_z = 0u;
    uint32_t best_owned_z = 0u;
    if (wm == NULL)
        return -1;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        const haeun_gfx_window *w = &wm->windows[i];
        if (!w->used || !w->visible || w->minimized)
            continue;
        if (best_any < 0 || w->z >= best_any_z) {
            best_any = i;
            best_any_z = w->z;
        }
        if (w->owner_pid > 0 &&
            (best_owned < 0 || w->z >= best_owned_z)) {
            best_owned = i;
            best_owned_z = w->z;
        }
    }
    return best_owned >= 0 ? best_owned : best_any;
}

/**
 * Ensures focus_id is eligible; retargets to top eligible or -1 (no-focus).
 */
haeun_gfx_result haeun_gfx_wm_focus_recover(haeun_gfx_wm *wm) {
    int cur;
    if (wm == NULL) {
        log_fail("wm_focus_recover bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    cur = wm->focus_id;
    if (cur >= 0 && cur < HAEUN_GFX_WM_MAX_WINDOWS) {
        const haeun_gfx_window *w = &wm->windows[cur];
        if (w->used && w->visible && !w->minimized)
            return HAEUN_GFX_OK;
    }
    wm->focus_id = haeun_gfx_wm_top_eligible(wm);
    return HAEUN_GFX_OK;
}

/**
 * Destroys a registered window slot and recovers focus.
 */
haeun_gfx_result haeun_gfx_wm_destroy(haeun_gfx_wm *wm, int id) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_destroy unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    memset(win, 0, sizeof(*win));
    return haeun_gfx_wm_focus_recover(wm);
}

/**
 * Updates screen-space origin for a window.
 */
haeun_gfx_result haeun_gfx_wm_move(haeun_gfx_wm *wm, int id, int32_t x,
                                   int32_t y) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_move unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    win->x = x;
    win->y = y;
    return HAEUN_GFX_OK;
}

/**
 * Sets window visibility for the compositor; recovers focus when hiding.
 * Showing clears minimized so used/visible/minimized stay consistent.
 */
haeun_gfx_result haeun_gfx_wm_set_visible(haeun_gfx_wm *wm, int id,
                                          int visible) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_set_visible unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (visible != 0) {
        win->visible = 1;
        win->minimized = 0;
        return HAEUN_GFX_OK;
    }
    win->visible = 0;
    return haeun_gfx_wm_focus_recover(wm);
}

/**
 * Assigns the next z value so the window composites on top.
 */
haeun_gfx_result haeun_gfx_wm_raise(haeun_gfx_wm *wm, int id) {
    haeun_gfx_window *win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_raise unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    win->z = wm->next_z++;
    return HAEUN_GFX_OK;
}

/**
 * Assigns focus to an eligible window id, or clears when id == -1.
 * Rejects minimized or invisible targets (never focus ineligible slots).
 */
haeun_gfx_result haeun_gfx_wm_set_focus(haeun_gfx_wm *wm, int id) {
    haeun_gfx_window *win;
    if (wm == NULL) {
        log_fail("wm_set_focus bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (id == -1) {
        wm->focus_id = -1;
        return HAEUN_GFX_OK;
    }
    win = slot(wm, id);
    if (win == NULL) {
        log_fail("wm_set_focus unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (!win->visible || win->minimized) {
        log_fail("wm_set_focus ineligible (min/hidden)");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    wm->focus_id = id;
    return HAEUN_GFX_OK;
}

/**
 * Picks the topmost visible window containing (sx,sy) and focuses it.
 * Guest-owned windows win over unowned shell surfaces at the same point.
 */
haeun_gfx_result haeun_gfx_wm_hit_focus(haeun_gfx_wm *wm, int32_t sx,
                                        int32_t sy, int *out_id) {
    int i;
    int best_any = -1;
    int best_owned = -1;
    uint32_t best_any_z = 0u;
    uint32_t best_owned_z = 0u;
    int best;
    if (wm == NULL) {
        log_fail("wm_hit_focus bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        haeun_gfx_window *w = &wm->windows[i];
        int32_t x1;
        int32_t y1;
        if (!w->used || !w->visible || w->minimized)
            continue;
        x1 = w->x + (int32_t)w->w;
        y1 = w->y + (int32_t)w->h;
        if (sx < w->x || sy < w->y || sx >= x1 || sy >= y1)
            continue;
        if (best_any < 0 || w->z >= best_any_z) {
            best_any = i;
            best_any_z = w->z;
        }
        if (w->owner_pid > 0 &&
            (best_owned < 0 || w->z >= best_owned_z)) {
            best_owned = i;
            best_owned_z = w->z;
        }
    }
    best = best_owned >= 0 ? best_owned : best_any;
    if (best < 0) {
        wm->focus_id = -1;
        if (out_id != NULL)
            *out_id = -1;
        return HAEUN_GFX_OK;
    }
    (void)haeun_gfx_wm_raise(wm, best);
    wm->focus_id = best;
    if (out_id != NULL)
        *out_id = best;
    return HAEUN_GFX_OK;
}

/**
 * Stores one RGBA pixel into the window backing in guest RAM.
 */
haeun_gfx_result haeun_gfx_wm_put_pixel(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, uint32_t rgba) {
    haeun_gfx_window *win = slot(wm, id);
    uint64_t off;
    if (win == NULL) {
        log_fail("wm_put_pixel unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (x >= win->w || y >= win->h) return HAEUN_GFX_OOB;
    off = ((uint64_t)y * (uint64_t)win->bw + (uint64_t)x) * 4u;
    store_rgba(wm->vm->ram + (size_t)win->base + (size_t)off, rgba);
    return HAEUN_GFX_OK;
}

/**
 * Fills the entire window backing with rgba.
 */
haeun_gfx_result haeun_gfx_wm_fill(haeun_gfx_wm *wm, int id, uint32_t rgba) {
    haeun_gfx_window *win = slot(wm, id);
    uint32_t x, y;
    if (win == NULL) {
        log_fail("wm_fill unknown id");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    for (y = 0u; y < win->bh; ++y) {
        for (x = 0u; x < win->bw; ++x) {
            uint64_t off = ((uint64_t)y * (uint64_t)win->bw + (uint64_t)x) * 4u;
            store_rgba(wm->vm->ram + (size_t)win->base + (size_t)off, rgba);
        }
    }
    return HAEUN_GFX_OK;
}
