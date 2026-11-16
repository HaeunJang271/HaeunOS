#ifndef HAEUN_GFX_WM_H
#define HAEUN_GFX_WM_H

#include "haeun/gfx.h"

#include <stdint.h>

enum {
    HAEUN_GFX_WM_MAX_WINDOWS = 8,
    HAEUN_GFX_WM_TITLE_MAX = 12,
    /*
     * Guest WIN_CREATE backing pool (process-v1). Host/unit-test creates may
     * use other RAM; user ECALL path enforces this range when pid != 0.
     */
    HAEUN_WIN_POOL_LO = 0x00018000u,
    HAEUN_WIN_POOL_HI = 0x00100000u
};

typedef struct {
    int used;
    int visible;
    int minimized;
    int maximized;
    int owner_pid; /* 0 = unowned / legacy single-process demos */
    int32_t x;
    int32_t y;
    uint32_t w;  /* display width (<= bw) */
    uint32_t h;  /* display height (<= bh) */
    uint32_t bw; /* immutable backing width */
    uint32_t bh; /* immutable backing height */
    uint32_t base;
    uint32_t z;
    int32_t rest_x;
    int32_t rest_y;
    uint32_t rest_w;
    uint32_t rest_h;
    char title[HAEUN_GFX_WM_TITLE_MAX];
} haeun_gfx_window;

typedef struct {
    haeun_vm *vm;
    haeun_gfx_window windows[HAEUN_GFX_WM_MAX_WINDOWS];
    uint32_t next_z;
    int focus_id;
} haeun_gfx_wm;

/** Clears the window table and binds it to a guest VM. */
haeun_gfx_result haeun_gfx_wm_init(haeun_gfx_wm *wm, haeun_vm *vm);

/** Registers a window; backing size becomes immutable bw/bh. */
haeun_gfx_result haeun_gfx_wm_create(haeun_gfx_wm *wm, int32_t x, int32_t y,
                                     uint32_t w, uint32_t h, uint32_t base,
                                     int *out_id);

/** Sets the owning process pid (0 clears ownership). */
haeun_gfx_result haeun_gfx_wm_set_owner(haeun_gfx_wm *wm, int id, int pid);

/**
 * Returns 1 when the window is used and owned by pid.
 * Unowned windows (owner_pid == 0) are host-only and return 0.
 */
int haeun_gfx_wm_owned_by(const haeun_gfx_wm *wm, int id, int pid);

/**
 * True when [base, base+w*h*4) is 4-aligned, inside RAM, outside the primary
 * framebuffer (when gfx is non-NULL and ready), and does not overlap another
 * used window's backing.
 */
int haeun_gfx_wm_backing_allowed(const haeun_gfx_wm *wm, const haeun_gfx *gfx,
                                 uint32_t base, uint32_t w, uint32_t h);

/**
 * True when base..end lies entirely in the guest window pool
 * [HAEUN_WIN_POOL_LO, HAEUN_WIN_POOL_HI).
 */
int haeun_gfx_wm_in_user_pool(uint32_t base, uint32_t w, uint32_t h);

/** Marks a slot unused and recovers focus to the next eligible window. */
haeun_gfx_result haeun_gfx_wm_destroy(haeun_gfx_wm *wm, int id);

/**
 * Returns the topmost used+visible+!minimized window id by z-order, or -1.
 */
int haeun_gfx_wm_top_eligible(const haeun_gfx_wm *wm);

/**
 * Ensures focus_id is -1 or an eligible window; retargets when invalid.
 * Documented no-focus (-1) when no eligible window remains.
 */
haeun_gfx_result haeun_gfx_wm_focus_recover(haeun_gfx_wm *wm);

/** Moves the window origin in screen space. */
haeun_gfx_result haeun_gfx_wm_move(haeun_gfx_wm *wm, int id, int32_t x,
                                   int32_t y);

/**
 * Sets display size clamped to backing and non-zero; does not realloc guest RAM.
 */
haeun_gfx_result haeun_gfx_wm_resize(haeun_gfx_wm *wm, int id, uint32_t w,
                                     uint32_t h);

/** Shows or hides a window without destroying its backing. */
haeun_gfx_result haeun_gfx_wm_set_visible(haeun_gfx_wm *wm, int id,
                                          int visible);

/** Raises a window to the top of the z-order. */
haeun_gfx_result haeun_gfx_wm_raise(haeun_gfx_wm *wm, int id);

/**
 * Sets keyboard/click focus to an eligible id, or clears when id is -1.
 * Rejects minimized or invisible windows.
 */
haeun_gfx_result haeun_gfx_wm_set_focus(haeun_gfx_wm *wm, int id);

/**
 * Hit-tests screen (sx,sy) against visible windows (highest z wins).
 * Skips minimized windows.
 */
haeun_gfx_result haeun_gfx_wm_hit_focus(haeun_gfx_wm *wm, int32_t sx,
                                        int32_t sy, int *out_id);

/** Copies a short title into the window metadata (host chrome). */
haeun_gfx_result haeun_gfx_wm_set_title(haeun_gfx_wm *wm, int id,
                                        const char *title);

/** Hides the window and clears focus if it was active. */
haeun_gfx_result haeun_gfx_wm_minimize(haeun_gfx_wm *wm, int id);

/**
 * Saves geometry and expands display rect to [0,0,fb_w,fb_h] clamped to backing.
 */
haeun_gfx_result haeun_gfx_wm_maximize(haeun_gfx_wm *wm, int id, uint32_t fb_w,
                                       uint32_t fb_h);

/** Restores geometry saved by maximize, or un-minimizes. */
haeun_gfx_result haeun_gfx_wm_restore(haeun_gfx_wm *wm, int id);

/** Writes one pixel into a window backing buffer. */
haeun_gfx_result haeun_gfx_wm_put_pixel(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, uint32_t rgba);

/** Fills a window backing with a solid color. */
haeun_gfx_result haeun_gfx_wm_fill(haeun_gfx_wm *wm, int id, uint32_t rgba);

/**
 * Fills a rectangle inside a window's display size (clipped to w/h).
 */
haeun_gfx_result haeun_gfx_wm_fill_rect(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, uint32_t w, uint32_t h,
                                        uint32_t rgba);

/** Clears the primary framebuffer then blits visible windows by ascending z. */
haeun_gfx_result haeun_gfx_compose(haeun_gfx *gfx, haeun_gfx_wm *wm,
                                   uint32_t desktop_rgba);

#endif
