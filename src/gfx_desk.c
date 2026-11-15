#include "haeun/gfx_desk.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-desk] security/validation failure: %s\n", what);
}

static haeun_gfx_result paint_strip(haeun_gfx_desktop *desk) {
    haeun_gfx_result r;
    int i;
    uint32_t btn_w;
    r = haeun_gfx_wm_fill(desk->wm, desk->strip_id, desk->strip_bg);
    if (r != HAEUN_GFX_OK) return r;
    if (desk->client_count <= 0) return HAEUN_GFX_OK;
    btn_w = desk->gfx->width / (uint32_t)desk->client_count;
    if (btn_w < 16u) btn_w = 16u;
    for (i = 0; i < desk->client_count; ++i) {
        char title[HAEUN_GFX_UI_TEXT_MAX];
        int32_t x = (int32_t)((uint32_t)i * btn_w);
        uint32_t w = btn_w;
        if (x + (int32_t)w > (int32_t)desk->gfx->width)
            w = (uint32_t)((int32_t)desk->gfx->width - x);
        if (w < 4u) continue;
        memcpy(title, desk->tasks[i].text, sizeof(title));
        r = haeun_gfx_ui_button_init(&desk->tasks[i], desk->strip_id, x, 2,
                                     w > 2u ? w - 2u : w, 12u, desk->strip_fg,
                                     desk->strip_btn, desk->strip_border, title);
        if (r != HAEUN_GFX_OK) return r;
        r = haeun_gfx_ui_draw_button(desk->wm, &desk->tasks[i]);
        if (r != HAEUN_GFX_OK) return r;
    }

    return HAEUN_GFX_OK;
}

/**
 * Builds root wallpaper and bottom task-strip windows over guest RAM.
 */
haeun_gfx_result haeun_gfx_desktop_init(haeun_gfx_desktop *desk, haeun_gfx *gfx,
                                        haeun_gfx_wm *wm, uint32_t root_base,
                                        uint32_t strip_base) {
    uint32_t root_h;
    haeun_gfx_result r;
    if (desk == NULL || gfx == NULL || wm == NULL) {
        log_fail("desktop_init bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (!haeun_gfx_fb_ready(gfx) || wm->vm != gfx->vm) {
        log_fail("desktop_init fb/wm not ready");
        return HAEUN_GFX_NOT_READY;
    }
    if (gfx->height <= HAEUN_GFX_DESK_STRIP_H) {
        log_fail("desktop_init fb too short for strip");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    memset(desk, 0, sizeof(*desk));
    desk->gfx = gfx;
    desk->wm = wm;
    desk->wallpaper = 0x335577FFu;
    desk->strip_bg = 0x111111FFu;
    desk->strip_fg = 0xFFFFFFFFu;
    desk->strip_btn = 0x445566FFu;
    desk->strip_border = 0x99AABBFFu;
    root_h = gfx->height - HAEUN_GFX_DESK_STRIP_H;
    r = haeun_gfx_wm_create(wm, 0, 0, gfx->width, root_h, root_base,
                            &desk->root_id);
    if (r != HAEUN_GFX_OK) return r;
    r = haeun_gfx_wm_create(wm, 0, (int32_t)root_h, gfx->width,
                            HAEUN_GFX_DESK_STRIP_H, strip_base, &desk->strip_id);
    if (r != HAEUN_GFX_OK) return r;
    /* Strip stays above wallpaper; clients will raise above both. */
    r = haeun_gfx_wm_raise(wm, desk->strip_id);
    if (r != HAEUN_GFX_OK) return r;
    desk->ready = 1;
    return HAEUN_GFX_OK;
}

/**
 * Adds a client window title to the strip (does not create the window).
 */
haeun_gfx_result haeun_gfx_desktop_add_client(haeun_gfx_desktop *desk,
                                              int win_id, const char *title) {
    int idx;
    size_t i;
    if (desk == NULL || !desk->ready || win_id < 0) {
        log_fail("add_client bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (win_id == desk->root_id || win_id == desk->strip_id) {
        log_fail("add_client refuses chrome window");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    if (desk->client_count >= HAEUN_GFX_DESK_MAX_CLIENTS) {
        log_fail("add_client table full");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    idx = desk->client_count;
    desk->client_ids[idx] = win_id;
    memset(&desk->tasks[idx], 0, sizeof(desk->tasks[idx]));
    if (title == NULL) title = "";
    for (i = 0; i + 1u < (size_t)HAEUN_GFX_UI_TEXT_MAX && title[i] != '\0';
         ++i)
        desk->tasks[idx].text[i] = title[i];
    desk->tasks[idx].text[i] = '\0';
    desk->client_count++;
    return HAEUN_GFX_OK;
}

/**
 * Repaints desktop chrome and composites into the primary framebuffer.
 */
haeun_gfx_result haeun_gfx_desktop_present(haeun_gfx_desktop *desk) {
    haeun_gfx_result r;
    if (desk == NULL || !desk->ready) {
        log_fail("present not ready");
        return HAEUN_GFX_NOT_READY;
    }
    r = haeun_gfx_wm_fill(desk->wm, desk->root_id, desk->wallpaper);
    if (r != HAEUN_GFX_OK) return r;
    r = paint_strip(desk);
    if (r != HAEUN_GFX_OK) return r;
    /* Keep strip above wallpaper but below raised clients. */
    r = haeun_gfx_wm_raise(desk->wm, desk->strip_id);
    if (r != HAEUN_GFX_OK) return r;
    return haeun_gfx_compose(desk->gfx, desk->wm, 0x000000FFu);
}

/**
 * Raises a client when a strip task button is clicked in screen space.
 */
haeun_gfx_result haeun_gfx_desktop_click(haeun_gfx_desktop *desk, int32_t sx,
                                         int32_t sy, int *out_id) {
    int32_t local_y;
    int i;
    if (desk == NULL || !desk->ready || out_id == NULL) {
        log_fail("click bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    *out_id = -1;
    local_y = sy - (int32_t)(desk->gfx->height - HAEUN_GFX_DESK_STRIP_H);
    if (local_y < 0 || sx < 0 || (uint32_t)sx >= desk->gfx->width)
        return HAEUN_GFX_OK;
    for (i = 0; i < desk->client_count; ++i) {
        int hit = 0;
        haeun_gfx_result r =
            haeun_gfx_ui_button_hit(&desk->tasks[i], sx, local_y, &hit);
        if (r != HAEUN_GFX_OK) return r;
        if (hit) {
            r = haeun_gfx_wm_raise(desk->wm, desk->client_ids[i]);
            if (r != HAEUN_GFX_OK) return r;
            *out_id = desk->client_ids[i];
            return HAEUN_GFX_OK;
        }
    }
    return HAEUN_GFX_OK;
}
