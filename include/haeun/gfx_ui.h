#ifndef HAEUN_GFX_UI_H
#define HAEUN_GFX_UI_H

#include "haeun/gfx_wm.h"

#include <stdint.h>

enum {
    HAEUN_GFX_UI_TEXT_MAX = 48,
    HAEUN_GFX_UI_GLYPH = 8,
    HAEUN_GFX_UI_LINE_GAP = 0
};

typedef struct {
    int win_id;
    int32_t x;
    int32_t y;
    uint32_t fg;
    uint32_t bg;
    char text[HAEUN_GFX_UI_TEXT_MAX];
} haeun_gfx_label;

typedef struct {
    int win_id;
    int32_t x;
    int32_t y;
    uint32_t w;
    uint32_t h;
    uint32_t fg;
    uint32_t bg;
    uint32_t border;
    char text[HAEUN_GFX_UI_TEXT_MAX];
} haeun_gfx_button;

/**
 * Copies text into a label descriptor (truncates to HAEUN_GFX_UI_TEXT_MAX-1).
 */
haeun_gfx_result haeun_gfx_ui_label_init(haeun_gfx_label *lab, int win_id,
                                         int32_t x, int32_t y, uint32_t fg,
                                         uint32_t bg, const char *text);

/** Paints the label text into the window backing. */
haeun_gfx_result haeun_gfx_ui_draw_label(haeun_gfx_wm *wm,
                                         const haeun_gfx_label *lab);

/**
 * Draws one 8×8 glyph into window-local (x,y). bg==0 → transparent background.
 * Non-printable (except handled by draw_text) uses '?' glyph.
 */
haeun_gfx_result haeun_gfx_ui_draw_char(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, char ch, uint32_t fg,
                                        uint32_t bg);

/**
 * Draws NUL-terminated ASCII into a window backing.
 * Supports '\\n' line breaks, auto-wrap at window width, vertical clip.
 * Out-of-bounds glyphs are clipped; Hangul is not supported. bg==0 → transparent.
 */
haeun_gfx_result haeun_gfx_ui_draw_text(haeun_gfx_wm *wm, int id, uint32_t x,
                                        uint32_t y, const char *text,
                                        uint32_t fg, uint32_t bg);

/**
 * Copies text into a button descriptor (truncates to HAEUN_GFX_UI_TEXT_MAX-1).
 */
haeun_gfx_result haeun_gfx_ui_button_init(haeun_gfx_button *btn, int win_id,
                                          int32_t x, int32_t y, uint32_t w,
                                          uint32_t h, uint32_t fg, uint32_t bg,
                                          uint32_t border, const char *text);

/** Paints fill, 1px border, and centered-left text into the window backing. */
haeun_gfx_result haeun_gfx_ui_draw_button(haeun_gfx_wm *wm,
                                          const haeun_gfx_button *btn);

/**
 * Sets *hit when (wx,wy) lies inside the button’s window-local rectangle.
 */
haeun_gfx_result haeun_gfx_ui_button_hit(const haeun_gfx_button *btn,
                                         int32_t wx, int32_t wy, int *hit);

#endif
