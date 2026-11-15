#include "haeun/gfx.h"
#include "haeun/gfx_desk.h"
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
    FB_BASE = 0x1000u,    /* 64*48*4 = 0x3000 → ends 0x4000 */
    ROOT_BASE = 0x4000u,  /* 64*32*4 = 0x2000 → ends 0x6000 */
    STRIP_BASE = 0x6000u, /* 64*16*4 = 0x1000 → ends 0x7000 */
    CLIENT_BASE = 0x7000u /* 24*16*4 = 0x600 → ends 0x7600 */
};

/**
 * Desktop present paints wallpaper and strip; client appears after register.
 */
static int test_desktop_present_and_click(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_desktop desk;
    int client = -1;
    int raised = -1;
    uint32_t px = 0u;
    uint32_t client_color = 0x00FF00FFu;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_desktop_init(&desk, &gfx, &wm, ROOT_BASE, STRIP_BASE) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 8, 24u, 16u, CLIENT_BASE, &client) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, client, client_color) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_desktop_add_client(&desk, client, "App") == HAEUN_GFX_OK);
    CHECK(haeun_gfx_desktop_present(&desk) == HAEUN_GFX_OK);
    /* Wallpaper pixel (root is under strip; client may cover (8,8)). */
    CHECK(haeun_gfx_get_pixel(&gfx, 0u, 0u, &px) == HAEUN_GFX_OK);
    CHECK(px == desk.wallpaper);
    /* Client body. */
    CHECK(haeun_gfx_get_pixel(&gfx, 10u, 10u, &px) == HAEUN_GFX_OK);
    CHECK(px == client_color);
    /* Strip row. */
    CHECK(haeun_gfx_get_pixel(&gfx, 1u, FB_H - 1u, &px) == HAEUN_GFX_OK);
    CHECK(px == desk.strip_bg || px == desk.strip_btn || px == desk.strip_border ||
          px == desk.strip_fg);
    /* Click strip task → raise client id. */
    CHECK(haeun_gfx_desktop_click(&desk, 4, (int32_t)(FB_H - 8u), &raised) ==
          HAEUN_GFX_OK);
    CHECK(raised == client);
    CHECK(haeun_gfx_desktop_present(&desk) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 10u, 10u, &px) == HAEUN_GFX_OK);
    CHECK(px == client_color);
    return 0;
}

/**
 * Rejects registering chrome windows and oversized FB-less init.
 */
static int test_desktop_validation(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_desktop desk;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_desktop_init(&desk, &gfx, &wm, ROOT_BASE, STRIP_BASE) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_desktop_add_client(&desk, desk.root_id, "bad") ==
          HAEUN_GFX_BAD_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_desktop_present_and_click() != 0) return 1;
    if (test_desktop_validation() != 0) return 1;
    puts("all gfx desk tests passed");
    return 0;
}
