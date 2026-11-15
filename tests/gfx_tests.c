#include "haeun/gfx.h"
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
    FB_W = 32u,
    FB_H = 16u,
    FB_BASE = 0x1000u
};

/**
 * Rejects enable when the buffer would leave RAM.
 */
static int test_enable_rejects_oob_base(void) {
    uint8_t ram[0x2000];
    haeun_vm vm;
    haeun_gfx gfx;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, 0x1F00u) ==
          HAEUN_GFX_BAD_ARGUMENT);
    CHECK(!haeun_gfx_fb_ready(&gfx));
    return 0;
}

/**
 * Draws pixels only after STATUS reports ready; disable blocks further draws.
 */
static int test_pixel_path(void) {
    uint8_t ram[0x4000];
    haeun_vm vm;
    haeun_gfx gfx;
    uint32_t px = 0u;
    memset(ram, 0, sizeof(ram));
    memset(&gfx, 0, sizeof(gfx));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_put_pixel(&gfx, 0u, 0u, 0xFF0000FFu) ==
          HAEUN_GFX_NOT_READY);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_fb_ready(&gfx));
    CHECK(haeun_gfx_clear(&gfx, 0x000000FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_put_pixel(&gfx, 1u, 2u, 0x11223344u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 1u, 2u, &px) == HAEUN_GFX_OK);
    CHECK(px == 0x11223344u);
    CHECK(haeun_gfx_put_pixel(&gfx, FB_W, 0u, 0u) == HAEUN_GFX_OOB);
    CHECK(haeun_gfx_fill_rect(&gfx, 4u, 4u, 3u, 2u, 0xAABBCCDDu) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 5u, 5u, &px) == HAEUN_GFX_OK);
    CHECK(px == 0xAABBCCDDu);
    CHECK(haeun_gfx_fb_disable(&gfx) == HAEUN_GFX_OK);
    CHECK(!haeun_gfx_fb_ready(&gfx));
    CHECK(haeun_gfx_put_pixel(&gfx, 0u, 0u, 0u) == HAEUN_GFX_NOT_READY);
    return 0;
}

/**
 * Font renderer writes distinct fg/bg into the guest framebuffer.
 */
static int test_font_string(void) {
    uint8_t ram[0x4000];
    haeun_vm vm;
    haeun_gfx gfx;
    uint32_t px = 0u;
    uint32_t fg = 0x00FF00FFu;
    uint32_t bg = 0x000000FFu;
    int found_fg = 0;
    uint32_t y, x;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_clear(&gfx, bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_draw_string(&gfx, 0u, 0u, "Hi", fg, bg) == HAEUN_GFX_OK);
    for (y = 0u; y < 8u; ++y) {
        for (x = 0u; x < 16u; ++x) {
            CHECK(haeun_gfx_get_pixel(&gfx, x, y, &px) == HAEUN_GFX_OK);
            if (px == fg) found_fg = 1;
            CHECK(px == fg || px == bg);
        }
    }
    CHECK(found_fg);
    CHECK(haeun_gfx_draw_char(&gfx, 0u, 8u, '\x01', fg, bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 0u, 8u, &px) == HAEUN_GFX_OK);
    CHECK(px == fg);
    return 0;
}

int main(void) {
    if (test_enable_rejects_oob_base() != 0) return 1;
    if (test_pixel_path() != 0) return 1;
    if (test_font_string() != 0) return 1;
    puts("all gfx tests passed");
    return 0;
}
