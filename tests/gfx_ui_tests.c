#include "haeun/gfx.h"
#include "haeun/gfx_ui.h"
#include "haeun/gfx_wm.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

enum {
    FB_W = 64u,
    FB_H = 48u,
    FB_BASE = 0x1000u,   /* 64*48*4 = 0x3000 bytes → ends 0x4000 */
    WIN_BASE = 0x4000u,  /* must not overlap the primary FB */
    WIN_W = 48u,
    WIN_H = 32u
};

/**
 * Label and button pixels reach the primary FB after compose; hit-test works.
 */
static int test_label_button_compose(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_label lab;
    haeun_gfx_button btn;
    int wid = -1;
    int hit = 0;
    uint32_t px = 0u;
    uint32_t desk = 0x000000FFu;
    uint32_t win_bg = 0x222222FFu;
    uint32_t fg = 0xFFFFFFFFu;
    uint32_t btn_bg = 0x004488FFu;
    uint32_t border = 0xFFFFFFFFu;
    int found_fg = 0;
    uint32_t x, y;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 4, WIN_W, WIN_H, WIN_BASE, &wid) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, wid, win_bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_label_init(&lab, wid, 2, 2, fg, win_bg, "Hi") ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_label(&wm, &lab) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_button_init(&btn, wid, 2, 14, 28u, 12u, fg, btn_bg,
                                   border, "OK") == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_button(&wm, &btn) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_button_hit(&btn, 5, 16, &hit) == HAEUN_GFX_OK);
    CHECK(hit == 1);
    CHECK(haeun_gfx_ui_button_hit(&btn, 40, 16, &hit) == HAEUN_GFX_OK);
    CHECK(hit == 0);
    CHECK(haeun_gfx_compose(&gfx, &wm, desk) == HAEUN_GFX_OK);
    /* Screen pos = window origin (4,4) + label (2,2) → (6,6). */
    for (y = 6u; y < 14u; ++y) {
        for (x = 6u; x < 22u; ++x) {
            CHECK(haeun_gfx_get_pixel(&gfx, x, y, &px) == HAEUN_GFX_OK);
            if (px == fg) found_fg = 1;
        }
    }
    CHECK(found_fg);
    /* Button border pixel on screen: (4+2, 4+14) = (6,18). */
    CHECK(haeun_gfx_get_pixel(&gfx, 6u, 18u, &px) == HAEUN_GFX_OK);
    CHECK(px == border);
    CHECK(haeun_gfx_get_pixel(&gfx, 8u, 20u, &px) == HAEUN_GFX_OK);
    CHECK(px == btn_bg || px == fg || px == border);
    return 0;
}

/**
 * Rejects undersized buttons and unknown draw targets via validation.
 */
static int test_ui_validation(void) {
    haeun_gfx_button btn;
    int hit = 1;
    CHECK(haeun_gfx_ui_button_init(&btn, 0, 0, 0, 2u, 2u, 0u, 0u, 0u, "x") ==
          HAEUN_GFX_BAD_ARGUMENT);
    CHECK(haeun_gfx_ui_button_hit(NULL, 0, 0, &hit) == HAEUN_GFX_BAD_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_label_button_compose() != 0) return 1;
    if (test_ui_validation() != 0) return 1;
    puts("all gfx ui tests passed");
    return 0;
}
