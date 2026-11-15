#ifndef HAEUN_GFX_CHROME_H
#define HAEUN_GFX_CHROME_H

#include "haeun/evt.h"
#include "haeun/gfx_wm.h"

#include <stdint.h>

enum {
    HAEUN_CHROME_TITLE_H = 6,
    HAEUN_CHROME_BORDER = 1,
    HAEUN_CHROME_BTN = 5
};

typedef struct {
    haeun_gfx *gfx;
    haeun_gfx_wm *wm;
    haeun_evt_hub *evt; /* optional M20 per-process event demux */
    int drag_id;
    int32_t drag_dx;
    int32_t drag_dy;
    int32_t drag_ox; /* window x at drag start (MOVED on release if changed) */
    int32_t drag_oy;
    int resize_id;
    int32_t resize_ox;
    int32_t resize_oy;
    uint32_t resize_ow;
    uint32_t resize_oh;
    uint32_t prev_buttons;
    int dirty;
    uint8_t last_key;
    int attached;
} haeun_gfx_chrome;

/** Binds chrome interaction state to an existing gfx+wm pair. */
haeun_gfx_result haeun_gfx_chrome_init(haeun_gfx_chrome *ch, haeun_gfx *gfx,
                                       haeun_gfx_wm *wm);

/**
 * Composes the desktop then paints title bars, borders, and caption buttons.
 */
haeun_gfx_result haeun_gfx_chrome_present(haeun_gfx_chrome *ch,
                                          uint32_t desktop_rgba);

/**
 * Handles mouse sample (screen coords, button mask). Rising edges click chrome;
 * held left button continues drag/resize.
 */
haeun_gfx_result haeun_gfx_chrome_pointer(haeun_gfx_chrome *ch, int32_t sx,
                                          int32_t sy, uint32_t buttons);

/** Records a scancode for the focused window (host-side key routing bookkeeping). */
haeun_gfx_result haeun_gfx_chrome_key(haeun_gfx_chrome *ch, uint8_t scancode);

#endif
