#include "haeun/ecall_mux.h"
#include "haeun/fb_dump.h"
#include "haeun/gfx.h"
#include "haeun/gfx_ecall.h"
#include "haeun/gfx_wm.h"
#include "haeun/sec.h"
#include "haeun/vm.h"
#include "haeun/vm_gui_host.h"

#include <stdio.h>
#include <stdlib.h>
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

/**
 * PPM header and RGB conversion from a known RGBA pattern.
 */
static int test_ppm_header_and_pixels(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx gfx;
    char path[] = "build/m7_pat.ppm";
    FILE *f;
    char header[64];
    uint8_t rgb[9];
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 3u, 1u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_put_pixel(&gfx, 0u, 0u, 0x000000FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_put_pixel(&gfx, 1u, 0u, 0x0000FF00u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_put_pixel(&gfx, 2u, 0u, 0x00FF0000u) == HAEUN_GFX_OK);
    CHECK(haeun_fb_dump_ppm_path(&vm, path) == HAEUN_FB_DUMP_OK);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fgets(header, sizeof(header), f) != NULL);
    CHECK(strcmp(header, "P6\n") == 0);
    CHECK(fgets(header, sizeof(header), f) != NULL);
    CHECK(strcmp(header, "3 1\n") == 0);
    CHECK(fgets(header, sizeof(header), f) != NULL);
    CHECK(strcmp(header, "255\n") == 0);
    CHECK(fread(rgb, 1u, 9u, f) == 9u);
    CHECK(rgb[0] == 0xFFu && rgb[1] == 0u && rgb[2] == 0u);
    CHECK(rgb[3] == 0u && rgb[4] == 0xFFu && rgb[5] == 0u);
    CHECK(rgb[6] == 0u && rgb[7] == 0u && rgb[8] == 0xFFu);
    (void)fclose(f);
    return 0;
}

/**
 * Rejects dump when BASE+size leaves RAM or dimensions are invalid.
 */
static int test_ppm_bounds(void) {
    uint8_t ram[0x2000];
    haeun_vm vm;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    vm.fb_control = 1u;
    vm.fb_width = 0u;
    vm.fb_height = 1u;
    vm.fb_base = 0u;
    CHECK(haeun_fb_dump_ppm_path(&vm, "build/m7_bad.ppm") ==
          HAEUN_FB_DUMP_BAD_ARGUMENT);
    vm.fb_width = 1025u;
    CHECK(haeun_fb_dump_ppm_path(&vm, "build/m7_bad.ppm") ==
          HAEUN_FB_DUMP_BAD_ARGUMENT);
    vm.fb_width = 64u;
    vm.fb_height = 64u;
    vm.fb_base = 0x1000u; /* 64*64*4 does not fit in 0x2000 RAM */
    CHECK(haeun_fb_dump_ppm_path(&vm, "build/m7_bad.ppm") == HAEUN_FB_DUMP_OOB);
    vm.fb_control = 0u;
    vm.fb_width = 1u;
    vm.fb_height = 1u;
    vm.fb_base = 0u;
    CHECK(haeun_fb_dump_ppm_path(&vm, "build/m7_bad.ppm") ==
          HAEUN_FB_DUMP_NOT_READY);
    return 0;
}

/**
 * Mux routes WIN_FILL to graphics; YIELD is not claimed.
 */
static int test_mux_dispatch(void) {
    uint8_t ram[0x8000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_ecall_ctx gfx_ctx;
    haeun_ecall_mux_ctx mux;
    uint32_t px = 0u;
    uint32_t id;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    put_le32(ram + 4u, ENC(0x01, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    gfx_ctx.gfx = &gfx;
    gfx_ctx.wm = &wm;
    memset(&mux, 0, sizeof(mux));
    mux.gfx_ctx = &gfx_ctx;
    haeun_vm_set_ecall_io(&vm, haeun_ecall_mux_io, &mux);
    vm.privilege = HAEUN_PRIV_USER;
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[2] = 0u;
    vm.r[3] = 0u;
    vm.r[4] = 8u;
    vm.r[5] = 8u;
    vm.r[6] = 0x2000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    id = vm.r[1];
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_FILL;
    vm.r[2] = id;
    vm.r[3] = 0x000000FFu;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_GFX_COMPOSE;
    vm.r[2] = 0u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 0u, 0u, &px) == HAEUN_GFX_OK);
    CHECK(px == 0x000000FFu);
    vm.pc = 0u;
    vm.csr_trapvec = 32u;
    vm.r[1] = HAEUN_SYS_YIELD;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.pc == 32u && vm.csr_cause == 3u);
    return 0;
}

/**
 * End-to-end: guest-like ECALL path via gui host + PPM RGB bars.
 */
static int test_gui_host_ppm(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_vm_gui_host host;
    char path[] = "build/m7_bars.ppm";
    FILE *f;
    char line[64];
    uint8_t rgb[3];
    uint32_t id;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    vm.privilege = HAEUN_PRIV_USER;
    CHECK(haeun_vm_gui_host_attach(&host, &vm) == HAEUN_GFX_OK);
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[2] = 0u;
    vm.r[3] = 0u;
    vm.r[4] = 16u;
    vm.r[5] = 16u;
    vm.r[6] = 0x18000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    id = vm.r[1];
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_FILL;
    vm.r[2] = id;
    vm.r[3] = 0x000000FFu;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[2] = 16u;
    vm.r[3] = 0u;
    vm.r[4] = 16u;
    vm.r[5] = 16u;
    vm.r[6] = 0x19000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    id = vm.r[1];
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_FILL;
    vm.r[2] = id;
    vm.r[3] = 0x0000FF00u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_CREATE;
    vm.r[2] = 32u;
    vm.r[3] = 0u;
    vm.r[4] = 16u;
    vm.r[5] = 16u;
    vm.r[6] = 0x1A000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    id = vm.r[1];
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_FILL;
    vm.r[2] = id;
    vm.r[3] = 0x00FF0000u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_GFX_COMPOSE;
    vm.r[2] = 0u;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(haeun_fb_dump_ppm_path(&vm, path) == HAEUN_FB_DUMP_OK);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fgets(line, sizeof(line), f) != NULL);
    CHECK(strcmp(line, "P6\n") == 0);
    CHECK(fgets(line, sizeof(line), f) != NULL);
    CHECK(strcmp(line, "64 32\n") == 0);
    CHECK(fgets(line, sizeof(line), f) != NULL);
    CHECK(strcmp(line, "255\n") == 0);
    {
        long data = ftell(f);
        CHECK(data >= 0);
        /* pixel (0,0) red */
        CHECK(fread(rgb, 1u, 3u, f) == 3u);
        CHECK(rgb[0] == 0xFFu && rgb[1] == 0u && rgb[2] == 0u);
        /* pixel (16,0) green */
        CHECK(fseek(f, data + (long)(16u * 3u), SEEK_SET) == 0);
        CHECK(fread(rgb, 1u, 3u, f) == 3u);
        CHECK(rgb[0] == 0u && rgb[1] == 0xFFu && rgb[2] == 0u);
        /* pixel (32,0) blue */
        CHECK(fseek(f, data + (long)(32u * 3u), SEEK_SET) == 0);
        CHECK(fread(rgb, 1u, 3u, f) == 3u);
        CHECK(rgb[0] == 0u && rgb[1] == 0u && rgb[2] == 0xFFu);
        /* pixel (48,0) black desk */
        CHECK(fseek(f, data + (long)(48u * 3u), SEEK_SET) == 0);
        CHECK(fread(rgb, 1u, 3u, f) == 3u);
        CHECK(rgb[0] == 0u && rgb[1] == 0u && rgb[2] == 0u);
    }
    (void)fclose(f);
    return 0;
}

int main(void) {
    if (test_ppm_header_and_pixels() != 0)
        return 1;
    if (test_ppm_bounds() != 0)
        return 1;
    if (test_mux_dispatch() != 0)
        return 1;
    if (test_gui_host_ppm() != 0)
        return 1;
    puts("all m7 integration tests passed");
    return 0;
}
