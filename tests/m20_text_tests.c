/**
 * M20.3: glyph pixels, newline/wrap/clip, ownership, bad VA/length.
 */
#include "haeun/gfx.h"
#include "haeun/gfx_ui.h"
#include "haeun/gfx_wm.h"
#include "haeun/guest_pcb.h"
#include "haeun/m20_ecall.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <string.h>

#define ENC(op, rd, ra, rb, imm)                                               \
    (((uint32_t)(op) << 26) | ((uint32_t)(rd) << 22) | ((uint32_t)(ra) << 18) | \
     ((uint32_t)(rb) << 14) | ((uint32_t)(imm) & 0x3ffffu))

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void put_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void set_pcb_pid(uint8_t *ram, uint32_t curr, uint32_t pid) {
    put_le32(ram + 0xA400u, curr);
    put_le32(ram + 0xA000u + curr * 0x100u + 4u, pid);
}

static uint32_t win_px(haeun_gfx_wm *wm, int id, uint32_t x, uint32_t y) {
    haeun_gfx_window *w = &wm->windows[id];
    size_t off = ((size_t)y * (size_t)w->bw + (size_t)x) * 4u;
    const uint8_t *p = wm->vm->ram + (size_t)w->base + off;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int test_glyph_pixels(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int id = -1;
    const uint8_t *rows;
    uint32_t fg = 0x00FF00FFu;
    uint32_t bg = 0x000000FFu;
    uint32_t row, col;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 32u, 16u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, id, bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_char(&wm, id, 0, 0, 'A', fg, bg) == HAEUN_GFX_OK);
    rows = haeun_gfx_font8_rows('A');
    for (row = 0; row < 8u; ++row) {
        for (col = 0; col < 8u; ++col) {
            int on = (rows[row] & (uint8_t)(1u << col)) != 0u;
            uint32_t px = win_px(&wm, id, col, row);
            CHECK(px == (on ? fg : bg));
        }
    }
    /* 'A' row0=0x0C → bit0-left columns 2..3 are foreground */
    CHECK(win_px(&wm, id, 2u, 0u) == fg);
    return 0;
}

static int test_newline_wrap_clip(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int id = -1;
    uint32_t fg = 0xFFFFFFFFu;
    uint32_t bg = 0x111111FFu;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    /* 16px wide → 2 glyphs/line before wrap */
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 16u, 24u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, id, bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_text(&wm, id, 0, 0, "AB\nC", fg, bg) ==
          HAEUN_GFX_OK);
    /* 'A' has ink at (2,0) with bit0-left font */
    CHECK(win_px(&wm, id, 2u, 0u) == fg);
    /* 'C' after newline at y=8 */
    CHECK(win_px(&wm, id, 2u, 8u) == fg);
    CHECK(haeun_gfx_wm_fill(&wm, id, bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_text(&wm, id, 0, 0, "ABCD", fg, bg) == HAEUN_GFX_OK);
    /* width 16 → wrap after 2 glyphs; 'C' at y=8 */
    CHECK(win_px(&wm, id, 2u, 8u) == fg);
    /* clip: draw at x=12 should not write past w=16 into foreign memory —
       only OOB skip; pixel at 12 still in window */
    CHECK(haeun_gfx_ui_draw_char(&wm, id, 12, 0, 'X', fg, bg) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_text(&wm, id, 0, 0, "", fg, bg) == HAEUN_GFX_OK);
    return 0;
}

static int test_transparent_and_replace(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int id = -1;
    uint32_t fill = 0x12345678u;
    uint32_t fg = 0xAABBCCFFu;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 16u, 16u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, id, fill) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_ui_draw_char(&wm, id, 0, 0, ' ', fg, 0u) == HAEUN_GFX_OK);
    /* space with transparent bg leaves fill */
    CHECK(win_px(&wm, id, 0, 0) == fill);
    CHECK(haeun_gfx_ui_draw_char(&wm, id, 0, 0, (char)1, fg, fill) ==
          HAEUN_GFX_OK);
    return 0;
}

static int test_ecall_security(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_m20_ecall_ctx ctx;
    int id_a = -1, id_b = -1;
    char longbuf[64];
    int i;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 24u, 16u, 0x20000u, &id_a) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 0, 24u, 16u, 0x22000u, &id_b) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id_a, 2);
    (void)haeun_gfx_wm_set_owner(&wm, id_b, 3);
    memset(&ctx, 0, sizeof(ctx));
    ctx.gfx = &gfx;
    ctx.wm = &wm;
    haeun_vm_set_ecall_io(&vm, haeun_m20_ecall_io, &ctx);
    set_pcb_pid(ram, 1u, 2u);
    vm.privilege = HAEUN_PRIV_USER;
    put_le32(ram + 0x300u, 0x00692148u); /* "Hi\0" little */
    /* foreign window */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_DRAW_TEXT;
    vm.r[2] = (uint32_t)id_b;
    vm.r[3] = 0u;
    vm.r[4] = 0u;
    vm.r[5] = 0xFFFFFFFFu;
    vm.r[6] = 0x000000FFu;
    vm.r[7] = 0x300u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    /* bad VA */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_DRAW_TEXT;
    vm.r[2] = (uint32_t)id_a;
    vm.r[7] = 0xfffffff0u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    /* negative origin */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_DRAW_TEXT;
    vm.r[3] = 0xffffffffu;
    vm.r[4] = 0u;
    vm.r[7] = 0x300u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    /* oversize string */
    for (i = 0; i < 63; ++i)
        longbuf[i] = 'A';
    longbuf[63] = '\0';
    memcpy(ram + 0x400u, longbuf, 64);
    /* force no NUL in first 48 bytes */
    memset(ram + 0x400u, 'Z', 48);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_DRAW_TEXT;
    vm.r[3] = 0u;
    vm.r[7] = 0x400u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xfffffffeu);
    /* good draw */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_DRAW_TEXT;
    vm.r[7] = 0x300u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    /* 'H' row0=0x33 → bit0-left ink at columns 0..1 and 4..5 */
    CHECK(win_px(&wm, id_a, 0u, 0u) == 0xFFFFFFFFu);
    /* bad window id */
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_DRAW_TEXT;
    vm.r[2] = 99u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    return 0;
}

int main(void) {
    if (test_glyph_pixels() != 0)
        return 1;
    if (test_newline_wrap_clip() != 0)
        return 1;
    if (test_transparent_and_replace() != 0)
        return 1;
    if (test_ecall_security() != 0)
        return 1;
    (void)printf("m20_text_tests ok\n");
    return 0;
}
