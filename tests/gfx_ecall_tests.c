#include "haeun/gfx.h"
#include "haeun/gfx_ecall.h"
#include "haeun/gfx_wm.h"
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

enum {
    FB_W = 32u,
    FB_H = 24u,
    FB_BASE = 0x1000u,
    WIN_BASE = 0x2000u
};

static void put_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/**
 * Guest ECALL create/fill/compose paints a window onto the framebuffer.
 */
static int test_ecall_win_compose(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx ctx;
    uint32_t px = 0u;
    uint32_t id;
    uint32_t red = 0x0000FF00u;
    uint32_t desk = 0x000000FFu;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    put_le32(ram + 4u, ENC(0x01, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, FB_W, FB_H, FB_BASE) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    ctx.gfx = &gfx;
    ctx.wm = &wm;
    haeun_vm_set_ecall_io(&vm, haeun_gfx_ecall_io, &ctx);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[2] = 2u;
    vm.r[3] = 2u;
    vm.r[4] = 8u;
    vm.r[5] = 8u;
    vm.r[6] = WIN_BASE;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 4u);
    id = vm.r[1];
    CHECK(id < HAEUN_GFX_WM_MAX_WINDOWS);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_FILL;
    vm.r[2] = id;
    vm.r[3] = red;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_GFX_COMPOSE;
    vm.r[2] = desk;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    CHECK(haeun_gfx_get_pixel(&gfx, 2u, 2u, &px) == HAEUN_GFX_OK);
    CHECK(px == red);
    CHECK(haeun_gfx_get_pixel(&gfx, 0u, 0u, &px) == HAEUN_GFX_OK);
    CHECK(px == desk);
    return 0;
}

/**
 * YIELD still traps when the GUI assist is installed.
 */
static int test_yield_still_traps(void) {
    uint8_t ram[64];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx ctx;
    memset(ram, 0, sizeof(ram));
    memset(&gfx, 0, sizeof(gfx));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    gfx.vm = &vm;
    ctx.gfx = &gfx;
    ctx.wm = &wm;
    vm.csr_trapvec = 32u;
    haeun_vm_set_ecall_io(&vm, haeun_gfx_ecall_io, &ctx);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_YIELD;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 32u && vm.csr_cause == 3u);
    return 0;
}

int main(void) {
    if (test_ecall_win_compose() != 0) return 1;
    if (test_yield_still_traps() != 0) return 1;
    puts("all gfx ecall tests passed");
    return 0;
}
