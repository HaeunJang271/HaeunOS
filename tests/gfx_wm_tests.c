#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/vm.h"

#include <stddef.h>
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
    FB_W = 32u,
    FB_H = 24u,
    FB_BASE = 0x1000u,
    WIN_A_BASE = 0x2000u,
    WIN_B_BASE = 0x2200u
};

/**
 * Rejects backing that would leave guest RAM.
 */
static int test_create_rejects_oob(void) {
    uint8_t ram[0x3000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 64u, 64u, 0x2F00u, &id) ==
          HAEUN_GFX_BAD_ARGUMENT);
    return 0;
}

/**
 * Higher-z window occludes the lower one after compose.
 */
static int test_zorder_compose(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    int a = -1, b = -1;
    uint32_t px = 0u;
    uint32_t desk = 0x000000FFu;
    uint32_t red = 0x0000FF00u;
    uint32_t blue = 0x00FF0000u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 2, 2, 8u, 8u, WIN_A_BASE, &a) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 4, 8u, 8u, WIN_B_BASE, &b) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, a, red) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, b, blue) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, desk) == HAEUN_GFX_OK);
    /* Overlap at (5,5) belongs to B (created later → higher z). */
    CHECK(haeun_gfx_get_pixel(&gfx, 5u, 5u, &px) == HAEUN_GFX_OK);
    CHECK(px == blue);
    /* A-only region still red. */
    CHECK(haeun_gfx_get_pixel(&gfx, 2u, 2u, &px) == HAEUN_GFX_OK);
    CHECK(px == red);
    /* Desktop outside windows. */
    CHECK(haeun_gfx_get_pixel(&gfx, 0u, 0u, &px) == HAEUN_GFX_OK);
    CHECK(px == desk);
    /* Raise A above B; overlap becomes red. */
    CHECK(haeun_gfx_wm_raise(&wm, a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, desk) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 5u, 5u, &px) == HAEUN_GFX_OK);
    CHECK(px == red);
    return 0;
}

/**
 * Hidden windows are skipped; destroy removes them from compose.
 */
static int test_hide_and_destroy(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    int a = -1;
    uint32_t px = 0u;
    uint32_t desk = 0x010101FFu;
    uint32_t green = 0x00FF00FFu;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, WIN_A_BASE, &a) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, a, green) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_visible(&wm, a, 0) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, desk) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 1u, 1u, &px) == HAEUN_GFX_OK);
    CHECK(px == desk);
    CHECK(haeun_gfx_wm_set_visible(&wm, a, 1) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_destroy(&wm, a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, desk) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 1u, 1u, &px) == HAEUN_GFX_OK);
    CHECK(px == desk);
    CHECK(haeun_gfx_wm_destroy(&wm, a) == HAEUN_GFX_BAD_ARGUMENT);
    return 0;
}

/** Click hit-test raises the top window and sets focus_id. */
static int test_hit_focus(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int a = -1, b = -1, hit = -1;
    enum { A = 0x1000u, B = 0x2000u };
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == -1);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 16u, 16u, A, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 8, 16u, 16u, B, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_hit_focus(&wm, 10, 10, &hit) == HAEUN_GFX_OK);
    CHECK(hit == b);
    CHECK(wm.focus_id == b);
    CHECK(haeun_gfx_wm_hit_focus(&wm, 100, 100, &hit) == HAEUN_GFX_OK);
    CHECK(hit == -1);
    CHECK(wm.focus_id == -1);
    CHECK(haeun_gfx_wm_set_focus(&wm, a) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == a);
    CHECK(haeun_gfx_wm_destroy(&wm, a) == HAEUN_GFX_OK);
    /* Destroy recovers focus to the remaining eligible window. */
    CHECK(wm.focus_id == b);
    CHECK(haeun_gfx_wm_destroy(&wm, b) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == -1);
    return 0;
}

/** Minimize/destroy retarget focus; never leave stale or minimized focus. */
static int test_focus_recover(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int a = -1, b = -1, c = -1;
    enum { A = 0x1000u, B = 0x2000u, C = 0x3000u };
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, A, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 4, 8u, 8u, B, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 8, 8u, 8u, C, &c) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_minimize(&wm, b) == HAEUN_GFX_OK);
    CHECK(wm.focus_id != b);
    CHECK(wm.focus_id == haeun_gfx_wm_top_eligible(&wm));
    CHECK(wm.focus_id == c || wm.focus_id == a);
    CHECK(!wm.windows[wm.focus_id].minimized);
    CHECK(wm.windows[wm.focus_id].visible);
    CHECK(haeun_gfx_wm_set_focus(&wm, c) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_destroy(&wm, c) == HAEUN_GFX_OK);
    CHECK(wm.focus_id != c);
    CHECK(wm.focus_id == a);
    CHECK(haeun_gfx_wm_minimize(&wm, a) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == -1);
    CHECK(haeun_gfx_wm_top_eligible(&wm) == -1);
    return 0;
}

/** fill_rect paints a sub-rectangle without touching outside pixels. */
static int test_fill_rect(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int a = -1;
    enum { BASE = 0x1000u };
    size_t ink;
    size_t corner;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, BASE, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, a, 0x00000000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill_rect(&wm, a, 2u, 2u, 3u, 2u, 0x0000FF00u) ==
          HAEUN_GFX_OK);
    ink = (size_t)BASE + ((size_t)2u * 8u + 2u) * 4u;
    corner = (size_t)BASE;
    CHECK(ram[ink + 1u] == 0xffu); /* G channel */
    CHECK(ram[corner] == 0u);
    return 0;
}

int main(void) {
    if (test_create_rejects_oob() != 0) return 1;
    if (test_zorder_compose() != 0) return 1;
    if (test_hide_and_destroy() != 0) return 1;
    if (test_hit_focus() != 0) return 1;
    if (test_focus_recover() != 0) return 1;
    if (test_fill_rect() != 0) return 1;
    puts("all gfx wm tests passed");
    return 0;
}
