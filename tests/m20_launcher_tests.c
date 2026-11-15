#include "haeun/app_env.h"
#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
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

enum { PCB_BASE = 0xA000u, PCB_STRIDE = 0x100u, PCB_NEXT = 0xA404u };

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void make_tiny_hxf(uint8_t *out, size_t *out_len) {
    uint8_t code[8] = {0, 0, 0, 0, 0x00, 0x00, 0x00, 0x04};
    size_t total = 96u + sizeof(code);
    memset(out, 0, total);
    out[0] = 'H';
    out[1] = 'X';
    out[2] = 'F';
    out[3] = '1';
    out[4] = 64;
    out[6] = 1;
    out[8] = 4;
    out[16] = 0x00;
    out[17] = 0x80;
    out[18] = 0x10;
    out[19] = 0x00;
    out[20] = 64;
    out[24] = 1;
    out[26] = 32;
    out[28] = (uint8_t)total;
    out[64] = 1;
    out[68] = 7;
    out[72] = 96;
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

static void prep_pcb_bank(uint8_t *ram, size_t ram_size) {
    int i;
    memset(ram, 0, ram_size);
    for (i = 0; i < 4; ++i)
        wr32(ram + PCB_BASE + (uint32_t)i * PCB_STRIDE, 0u);
    wr32(ram + PCB_NEXT, 2u);
    wr32(ram + 0xA400u, 0u);
    wr32(ram + PCB_BASE + 4u, 1u); /* launcher pid 1 */
    wr32(ram + PCB_BASE + 0u, 2u); /* RUNNING */
}

static int test_registry_meta(void) {
    haeun_app_env env;
    haeun_vm vm;
    uint8_t ram[256 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    char buf[32];
    uint32_t flags = 0u;
    prep_pcb_bank(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "calc", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "calc", img, len) == HAEUN_APP_INVALID);
    CHECK(haeun_app_set_meta(&env, 0, "add demo", "1.0", "demo",
                             HAEUN_APP_FLAG_SINGLE) == HAEUN_APP_OK);
    CHECK(haeun_app_info(&env, 0, HAEUN_APP_FIELD_DESC, buf, sizeof(buf),
                         NULL) == HAEUN_APP_OK);
    CHECK(strcmp(buf, "add demo") == 0);
    CHECK(haeun_app_info(&env, 0, HAEUN_APP_FIELD_FLAGS, NULL, 0u, &flags) ==
          HAEUN_APP_OK);
    CHECK(flags == HAEUN_APP_FLAG_SINGLE);
    CHECK(haeun_app_info(&env, 9, HAEUN_APP_FIELD_NAME, buf, sizeof(buf),
                         NULL) == HAEUN_APP_NOENT);
    return 0;
}

static int test_spawn_run_ctrl(void) {
    haeun_app_env env;
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_evt_hub evt;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u;
    haeun_app_run_info ri;
    int wid = -1;
    haeun_evt ev;
    prep_pcb_bank(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&evt, &wm);
    CHECK(haeun_app_env_init(&env, &vm, &wm) == HAEUN_APP_OK);
    env.evt = &evt;
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_register(&env, "b", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(p1 >= 2u);
    CHECK(haeun_app_run_at(&env, 1, &ri) == HAEUN_APP_OK);
    CHECK(ri.pid == p1);
    CHECK(ri.app_index == 0);
    CHECK(haeun_app_spawn(&env, 1, &p2) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_NOSPC);
    CHECK(haeun_gfx_wm_create(&wm, 4, 4, 8u, 8u, 0x20000u, &wid) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, wid, (int)p2) == HAEUN_GFX_OK);
    CHECK(haeun_app_ctrl(&env, HAEUN_APP_CTRL_FOCUS, p2, 1u) == HAEUN_APP_OK);
    CHECK(wm.focus_id == wid);
    CHECK(haeun_app_ctrl(&env, HAEUN_APP_CTRL_CLOSE, p2, 1u) == HAEUN_APP_OK);
    {
        int saw_close = 0;
        int n;
        for (n = 0; n < 8; ++n) {
            if (haeun_evt_poll(&evt, (int)p2, &ev) != 1)
                break;
            if (ev.type == HAEUN_EVT_CLOSE)
                saw_close = 1;
        }
        CHECK(saw_close == 1);
    }
    CHECK(haeun_app_ctrl(&env, HAEUN_APP_CTRL_FOCUS, p2, 99u) ==
          HAEUN_APP_INVALID);
    CHECK(haeun_app_reap(&env, p1) == HAEUN_APP_OK);
    CHECK(haeun_app_run_at(&env, 1, &ri) == HAEUN_APP_NOENT);
    /* Reap clears runs[]; WAIT frees PCB — unit test frees slot PCB manually. */
    wr32(ram + PCB_BASE + 1u * PCB_STRIDE, 0u);
    wr32(ram + PCB_BASE + 1u * PCB_STRIDE + 4u, 0u);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    return 0;
}

static int test_single_instance(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u;
    prep_pcb_bank(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "solo", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_set_meta(&env, 0, "one", "1", "sys", HAEUN_APP_FLAG_SINGLE) ==
          HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p2) == HAEUN_APP_BUSY);
    return 0;
}

static int test_ecall_info_run(void) {
    haeun_app_env env;
    haeun_vm vm;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256];
    size_t len = 0u;
    uint32_t pid = 0u;
    prep_pcb_bank(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    vm.privilege = HAEUN_PRIV_USER;
    CHECK(haeun_app_env_init(&env, &vm, NULL) == HAEUN_APP_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "x", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_set_meta(&env, 0, "desc", "1.0", "cat", 0u) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &pid) == HAEUN_APP_OK);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_APP_INFO);
    haeun_rf_write(&vm, 2u, 0u);
    haeun_rf_write(&vm, 3u, (uint32_t)HAEUN_APP_FIELD_FLAGS);
    CHECK(haeun_app_ecall_io(&vm, &env));
    CHECK(haeun_rf_read(&vm, 1u) == 0u);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_APP_RUN);
    haeun_rf_write(&vm, 2u, 1u);
    CHECK(haeun_app_ecall_io(&vm, &env));
    CHECK(haeun_rf_read(&vm, 1u) == 0u);
    CHECK(haeun_rf_read(&vm, 2u) == pid);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_APP_RUN);
    haeun_rf_write(&vm, 2u, 0u);
    CHECK(haeun_app_ecall_io(&vm, &env));
    CHECK(haeun_rf_read(&vm, 1u) == 0xffffffffu);
    return 0;
}

int main(void) {
    if (test_registry_meta() != 0)
        return 1;
    if (test_spawn_run_ctrl() != 0)
        return 1;
    if (test_single_instance() != 0)
        return 1;
    if (test_ecall_info_run() != 0)
        return 1;
    puts("m20_launcher_tests ok");
    return 0;
}
