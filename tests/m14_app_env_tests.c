#include "haeun/app_env.h"
#include "haeun/gfx.h"
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

/* Minimal valid app HXF: one NOP+HALT at 0x108000 (built as raw header). */
static void make_tiny_hxf(uint8_t *out, size_t *out_len) {
    uint8_t code[8] = {0, 0, 0, 0, 0x00, 0x00, 0x00, 0x04}; /* nop; halt */
    size_t total = 96u + sizeof(code);
    memset(out, 0, total);
    out[0] = 'H';
    out[1] = 'X';
    out[2] = 'F';
    out[3] = '1';
    out[4] = 64;
    out[6] = 1;
    out[8] = 4; /* ISA 4 */
    out[16] = 0x00;
    out[17] = 0x80;
    out[18] = 0x10;
    out[19] = 0x00; /* entry 0x108000 */
    out[20] = 64;
    out[24] = 1;
    out[26] = 32;
    out[28] = (uint8_t)total;
    out[64] = 1; /* type */
    out[68] = 7; /* flags RWX */
    out[72] = 96; /* file off */
    out[76] = 0x00;
    out[77] = 0x80;
    out[78] = 0x10;
    out[79] = 0x00;
    out[80] = (uint8_t)sizeof(code);
    out[84] = (uint8_t)sizeof(code);
    out[88] = 4;
    memcpy(out + 96, code, sizeof(code));
    *out_len = total;
}

static int test_validate_and_register(void) {
    haeun_app_env env;
    haeun_vm vm;
    uint8_t ram[256 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    char name[16];
    uint8_t bad[8] = {0};
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "tiny", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_count(&env) == 1);
    CHECK(haeun_app_name(&env, 0, name, sizeof(name)) == HAEUN_APP_OK);
    CHECK(strcmp(name, "tiny") == 0);
    CHECK(haeun_app_register(&env, "bad", bad, sizeof(bad)) == HAEUN_APP_INVALID);
    return 0;
}

static int test_window_ownership(void) {
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    uint8_t ram[64 * 1024];
    int id = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 32u, 24u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x2000u, &id) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, id, 7) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_owned_by(&wm, id, 7) == 1);
    CHECK(haeun_gfx_wm_owned_by(&wm, id, 8) == 0);
    CHECK(haeun_app_destroy_windows(&wm, 7u) == HAEUN_APP_OK);
    CHECK(wm.windows[id].used == 0);
    return 0;
}

int main(void) {
    if (test_validate_and_register() != 0)
        return 1;
    if (test_window_ownership() != 0)
        return 1;
    puts("m14_app_env_tests ok");
    return 0;
}
