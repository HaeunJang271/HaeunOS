#include "haeun/app_env.h"
#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/svc_mgr.h"
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

static void prep_pcb(uint8_t *ram, size_t n) {
    int i;
    memset(ram, 0, n);
    for (i = 0; i < 4; ++i)
        wr32(ram + PCB_BASE + (uint32_t)i * PCB_STRIDE, 0u);
    wr32(ram + PCB_NEXT, 2u);
    wr32(ram + 0xA400u, 0u);
    wr32(ram + PCB_BASE + 4u, 1u);
    wr32(ram + PCB_BASE + 0u, 2u);
}

static int test_registry_and_start(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_app_env apps;
    haeun_sm_hub sm;
    haeun_evt_hub evt;
    uint8_t img[256];
    size_t len = 0u;
    char name[16];
    haeun_sm_stat st;
    uint32_t flags = 0u;
    prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&apps, &vm, NULL) == HAEUN_APP_OK);
    haeun_evt_hub_init(&evt, NULL);
    apps.evt = &evt;
    CHECK(haeun_sm_init(&sm, &vm, &apps, &evt) == HAEUN_SM_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&apps, "idle", img, len) == HAEUN_APP_OK);
    CHECK(haeun_sm_register(&sm, "idle", "d", "1", 0, HAEUN_SM_START_MANUAL,
                            HAEUN_SM_RESTART_NEVER) == HAEUN_SM_OK);
    CHECK(haeun_sm_register(&sm, "idle", "d", "1", 0, 0, 0) == HAEUN_SM_INVALID);
    CHECK(haeun_sm_register(&sm, "x", "d", "1", 9, 0, 0) == HAEUN_SM_INVALID);
    CHECK(haeun_sm_count(&sm) == 1);
    CHECK(haeun_sm_info(&sm, 0, HAEUN_SM_FIELD_NAME, name, sizeof(name),
                        NULL) == HAEUN_SM_OK);
    CHECK(strcmp(name, "idle") == 0);
    CHECK(haeun_sm_info(&sm, 0, HAEUN_SM_FIELD_FLAGS, NULL, 0u, &flags) ==
          HAEUN_SM_OK);
    CHECK(haeun_sm_start(&sm, 0) == HAEUN_SM_OK);
    CHECK(haeun_sm_status(&sm, 0, &st) == HAEUN_SM_OK);
    CHECK(st.state == HAEUN_SM_RUNNING);
    CHECK(st.pid >= 2u);
    CHECK(haeun_sm_start(&sm, 0) == HAEUN_SM_BUSY);
    CHECK(haeun_sm_start(&sm, 9) == HAEUN_SM_NOENT);
    return 0;
}

static int test_stop_and_exit(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_app_env apps;
    haeun_sm_hub sm;
    haeun_evt_hub evt;
    haeun_evt ev;
    uint8_t img[256];
    size_t len = 0u;
    haeun_sm_stat st;
    uint32_t pid;
    prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&apps, &vm, NULL) == HAEUN_APP_OK);
    haeun_evt_hub_init(&evt, NULL);
    apps.evt = &evt;
    CHECK(haeun_sm_init(&sm, &vm, &apps, &evt) == HAEUN_SM_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&apps, "idle", img, len) == HAEUN_APP_OK);
    CHECK(haeun_sm_register(&sm, "idle", "d", "1", 0, 0, HAEUN_SM_RESTART_NEVER) ==
          HAEUN_SM_OK);
    CHECK(haeun_sm_start(&sm, 0) == HAEUN_SM_OK);
    pid = sm.tab[0].pid;
    CHECK(haeun_sm_stop(&sm, 0, 1u) == HAEUN_SM_OK);
    CHECK(sm.tab[0].state == HAEUN_SM_STOPPING);
    CHECK(haeun_evt_poll(&evt, (int)pid, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_CLOSE);
    haeun_sm_on_pid_exit(&sm, pid, 0u, 0);
    CHECK(haeun_sm_status(&sm, 0, &st) == HAEUN_SM_OK);
    CHECK(st.state == HAEUN_SM_STOPPED);
    CHECK(st.pid == 0u);
    return 0;
}

static int test_restart_policy(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_app_env apps;
    haeun_sm_hub sm;
    haeun_evt_hub evt;
    uint8_t img[256];
    size_t len = 0u;
    haeun_sm_stat st;
    uint32_t pid;
    prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&apps, &vm, NULL) == HAEUN_APP_OK);
    haeun_evt_hub_init(&evt, NULL);
    CHECK(haeun_sm_init(&sm, &vm, &apps, &evt) == HAEUN_SM_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&apps, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_sm_register(&sm, "a", "d", "1", 0, 0, HAEUN_SM_RESTART_ON_FAIL) ==
          HAEUN_SM_OK);
    CHECK(haeun_sm_start(&sm, 0) == HAEUN_SM_OK);
    pid = sm.tab[0].pid;
    /* Simulate unexpected exit; PCB must be FREE for deferred restart. */
    wr32(ram + PCB_BASE + 1u * PCB_STRIDE, 0u);
    wr32(ram + PCB_BASE + 1u * PCB_STRIDE + 4u, 0u);
    (void)haeun_app_reap(&apps, pid);
    haeun_sm_on_pid_exit(&sm, pid, 5u, 1);
    CHECK(sm.tab[0].pending_restart == 1u);
    CHECK(haeun_sm_status(&sm, 0, &st) == HAEUN_SM_OK);
    CHECK(st.state == HAEUN_SM_RUNNING);
    CHECK(st.pid != pid);
    CHECK(st.restart_count >= 1u);
    return 0;
}

static int test_nospace(void) {
    static uint8_t ram[2 * 1024 * 1024];
    haeun_vm vm;
    haeun_app_env apps;
    haeun_sm_hub sm;
    uint8_t img[256];
    size_t len = 0u;
    uint32_t p1 = 0u, p2 = 0u;
    prep_pcb(ram, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_app_env_init(&apps, &vm, NULL) == HAEUN_APP_OK);
    CHECK(haeun_sm_init(&sm, &vm, &apps, NULL) == HAEUN_SM_OK);
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&apps, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_sm_register(&sm, "s0", "d", "1", 0, 0, 0) == HAEUN_SM_OK);
    CHECK(haeun_sm_register(&sm, "s1", "d", "1", 0, 0, 0) == HAEUN_SM_OK);
    CHECK(haeun_app_spawn(&apps, 0, &p1) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&apps, 0, &p2) == HAEUN_APP_OK);
    CHECK(haeun_sm_start(&sm, 0) == HAEUN_SM_NOSPC);
    return 0;
}

int main(void) {
    if (test_registry_and_start() != 0)
        return 1;
    if (test_stop_and_exit() != 0)
        return 1;
    if (test_restart_policy() != 0)
        return 1;
    if (test_nospace() != 0)
        return 1;
    puts("m20_service_tests ok");
    return 0;
}
