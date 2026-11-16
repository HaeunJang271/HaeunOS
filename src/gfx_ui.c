#include "haeun/gfx_ui.h"

#include <stdio.h>
#include <string.h>

static void log_fail(const char *what) {
    fprintf(stderr, "[haeun-gfx-ui] security/validation failure: %s\n", what);
}

static void copy_text(char *dst, const char *text) {
    size_t i;
    if (text == NULL)
        text = "";
    for (i = 0; i + 1u < (size_t)HAEUN_GFX_UI_TEXT_MAX && text[i] != '\0';
         ++i)
        dst[i] = text[i];
    dst[i] = '\0';
}

static haeun_gfx_result fill_rect_win(haeun_gfx_wm *wm, int id, int32_t x,
                                      int32_t y, uint32_t w, uint32_t h,
                                      uint32_t rgba) {
    uint32_t xi, yi;
    if (w == 0u || h == 0u)
        return HAEUN_GFX_OK;
    if (x < 0 || y < 0) {
        log_fail("widget rect negative origin");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    for (yi = 0u; yi < h; ++yi) {
        for (xi = 0u; xi < w; ++xi) {
            haeun_gfx_result r = haeun_gfx_wm_put_pixel(
                wm, id, (uint32_t)x + xi, (uint32_t)y + yi, rgba);
            if (r == HAEUN_GFX_OOB)
                continue;
            if (r != HAEUN_GFX_OK)
                return r;
        }
    }
    return HAEUN_GFX_OK;
}

/**
 * Draws one 8×8 glyph into window-local (x,y). bg==0 → transparent.
 */
haeun_gfx_result haeun_gfx_ui_draw_char(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, char ch, uint32_t fg,
                                        uint32_t bg) {
    const uint8_t *rows;
    unsigned u = (unsigned char)ch;
    uint32_t row, col;
    char draw = ch;
    if (wm == NULL || id < 0)
        return HAEUN_GFX_BAD_ARGUMENT;
    if (u < 32u || u > 126u)
        draw = '?';
    rows = haeun_gfx_font8_rows(draw);
    for (row = 0u; row < (uint32_t)HAEUN_GFX_UI_GLYPH; ++row) {
        uint8_t bits = rows[row];
        for (col = 0u; col < (uint32_t)HAEUN_GFX_UI_GLYPH; ++col) {
            int on = (bits & (uint8_t)(1u << col)) != 0u;
            haeun_gfx_result r;
            if (!on && bg == 0u)
                continue;
            r = haeun_gfx_wm_put_pixel(wm, id, x + col, y + row,
                                       on ? fg : bg);
            if (r == HAEUN_GFX_OOB)
                continue;
            if (r != HAEUN_GFX_OK)
                return r;
        }
    }
    return HAEUN_GFX_OK;
}

static haeun_gfx_result draw_text_win(haeun_gfx_wm *wm, int id, uint32_t x,
                                      uint32_t y, const char *text, uint32_t fg,
                                      uint32_t bg) {
    haeun_gfx_window *win;
    uint32_t cx = x;
    uint32_t cy = y;
    uint32_t origin_x = x;
    uint32_t gw = (uint32_t)HAEUN_GFX_UI_GLYPH;
    uint32_t lh = gw + (uint32_t)HAEUN_GFX_UI_LINE_GAP;
    if (wm == NULL || text == NULL || id < 0 ||
        id >= HAEUN_GFX_WM_MAX_WINDOWS || !wm->windows[id].used)
        return HAEUN_GFX_BAD_ARGUMENT;
    win = &wm->windows[id];
    while (*text != '\0') {
        char ch = *text++;
        if (ch == '\r')
            continue;
        if (ch == '\n') {
            cx = origin_x;
            cy += lh;
            continue;
        }
        if (cx + gw > win->w && cx > origin_x) {
            cx = origin_x;
            cy += lh;
        }
        if (cy >= win->h)
            break;
        if (cy + gw > win->h) {
            /* partial vertical clip via put_pixel OOB */
        }
        {
            haeun_gfx_result r =
                haeun_gfx_ui_draw_char(wm, id, cx, cy, ch, fg, bg);
            if (r != HAEUN_GFX_OK && r != HAEUN_GFX_OOB)
                return r;
        }
        cx += gw;
    }
    return HAEUN_GFX_OK;
}

/** Initializes a label with a copied text buffer. */
haeun_gfx_result haeun_gfx_ui_label_init(haeun_gfx_label *lab, int win_id,
                                         int32_t x, int32_t y, uint32_t fg,
                                         uint32_t bg, const char *text) {
    if (lab == NULL || win_id < 0) {
        log_fail("label_init bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    memset(lab, 0, sizeof(*lab));
    lab->win_id = win_id;
    lab->x = x;
    lab->y = y;
    lab->fg = fg;
    lab->bg = bg;
    copy_text(lab->text, text);
    return HAEUN_GFX_OK;
}

/** Draws a NUL-terminated ASCII string into a window backing (clips OOB). */
haeun_gfx_result haeun_gfx_ui_draw_text(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, const char *text,
                                        uint32_t fg, uint32_t bg) {
    if (wm == NULL || text == NULL || id < 0) {
        log_fail("draw_text bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    return draw_text_win(wm, id, x, y, text, fg, bg);
}

/** Draws label text into the target window backing. */
haeun_gfx_result haeun_gfx_ui_draw_label(haeun_gfx_wm *wm,
                                         const haeun_gfx_label *lab) {
    if (wm == NULL || lab == NULL || lab->x < 0 || lab->y < 0) {
        log_fail("draw_label bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    return draw_text_win(wm, lab->win_id, (uint32_t)lab->x, (uint32_t)lab->y,
                         lab->text, lab->fg, lab->bg);
}

/** Initializes a button with geometry and copied text. */
haeun_gfx_result haeun_gfx_ui_button_init(haeun_gfx_button *btn, int win_id,
                                          int32_t x, int32_t y, uint32_t w,
                                          uint32_t h, uint32_t fg, uint32_t bg,
                                          uint32_t border, const char *text) {
    if (btn == NULL || win_id < 0 || w < 4u || h < 4u) {
        log_fail("button_init bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    memset(btn, 0, sizeof(*btn));
    btn->win_id = win_id;
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->fg = fg;
    btn->bg = bg;
    btn->border = border;
    copy_text(btn->text, text);
    return HAEUN_GFX_OK;
}

/** Paints a button (fill, border, text) into the window backing. */
haeun_gfx_result haeun_gfx_ui_draw_button(haeun_gfx_wm *wm,
                                          const haeun_gfx_button *btn) {
    haeun_gfx_result r;
    uint32_t tx, ty;
    if (wm == NULL || btn == NULL || btn->x < 0 || btn->y < 0) {
        log_fail("draw_button bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    r = fill_rect_win(wm, btn->win_id, btn->x, btn->y, btn->w, btn->h,
                      btn->bg);
    if (r != HAEUN_GFX_OK)
        return r;
    r = fill_rect_win(wm, btn->win_id, btn->x, btn->y, btn->w, 1u, btn->border);
    if (r != HAEUN_GFX_OK)
        return r;
    r = fill_rect_win(wm, btn->win_id, btn->x, btn->y + (int32_t)btn->h - 1,
                      btn->w, 1u, btn->border);
    if (r != HAEUN_GFX_OK)
        return r;
    r = fill_rect_win(wm, btn->win_id, btn->x, btn->y, 1u, btn->h, btn->border);
    if (r != HAEUN_GFX_OK)
        return r;
    r = fill_rect_win(wm, btn->win_id, btn->x + (int32_t)btn->w - 1, btn->y, 1u,
                      btn->h, btn->border);
    if (r != HAEUN_GFX_OK)
        return r;
    tx = (uint32_t)btn->x + 2u;
    ty = (uint32_t)btn->y + (btn->h > 8u ? (btn->h - 8u) / 2u : 0u);
    return draw_text_win(wm, btn->win_id, tx, ty, btn->text, btn->fg, btn->bg);
}

/** Hit-tests window-local coordinates against the button rectangle. */
haeun_gfx_result haeun_gfx_ui_button_hit(const haeun_gfx_button *btn,
                                         int32_t wx, int32_t wy, int *hit) {
    if (btn == NULL || hit == NULL) {
        log_fail("button_hit bad argument");
        return HAEUN_GFX_BAD_ARGUMENT;
    }
    *hit = 0;
    if (wx < btn->x || wy < btn->y)
        return HAEUN_GFX_OK;
    if (wx >= btn->x + (int32_t)btn->w || wy >= btn->y + (int32_t)btn->h)
        return HAEUN_GFX_OK;
    *hit = 1;
    return HAEUN_GFX_OK;
}
