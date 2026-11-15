#ifndef HAEUN_GFX_DESK_H
#define HAEUN_GFX_DESK_H

#include "haeun/gfx_ui.h"

#include <stdint.h>

enum {
    HAEUN_GFX_DESK_STRIP_H = 16u,
    HAEUN_GFX_DESK_MAX_CLIENTS = 4
};

typedef struct {
    haeun_gfx *gfx;
    haeun_gfx_wm *wm;
    int root_id;
    int strip_id;
    uint32_t wallpaper;
    uint32_t strip_bg;
    uint32_t strip_fg;
    uint32_t strip_btn;
    uint32_t strip_border;
    int client_ids[HAEUN_GFX_DESK_MAX_CLIENTS];
    haeun_gfx_button tasks[HAEUN_GFX_DESK_MAX_CLIENTS];
    int client_count;
    int ready;
} haeun_gfx_desktop;

/**
 * Creates root and strip windows using guest-RAM backings; FB must already be ready.
 * root_base covers width*(height-STRIP_H)*4; strip_base covers width*STRIP_H*4.
 */
haeun_gfx_result haeun_gfx_desktop_init(haeun_gfx_desktop *desk, haeun_gfx *gfx,
                                        haeun_gfx_wm *wm, uint32_t root_base,
                                        uint32_t strip_base);

/** Registers an existing WM window on the task strip (copies title). */
haeun_gfx_result haeun_gfx_desktop_add_client(haeun_gfx_desktop *desk,
                                              int win_id, const char *title);

/** Repaints root/strip chrome and composites all visible windows. */
haeun_gfx_result haeun_gfx_desktop_present(haeun_gfx_desktop *desk);

/**
 * If (sx,sy) hits a strip task button, raises that client and sets *out_id.
 * *out_id is -1 when the click misses the strip tasks.
 */
haeun_gfx_result haeun_gfx_desktop_click(haeun_gfx_desktop *desk, int32_t sx,
                                         int32_t sy, int *out_id);

#endif
