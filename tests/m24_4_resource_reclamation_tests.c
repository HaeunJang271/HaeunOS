/**
 * M24.4 C/E: host resource reclaim once; PID reuse has no stale WM/evt/IPC.
 */
#include "haeun/app_env.h"
#include "haeun/ecall_mux.h"
#include "haeun/evt.h"
#include "haeun/exception_handler.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/ipc.h"
#include "haeun/m20_ecall.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"
#include "haeun/svc.h"
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

static void prep(uint8_t *ram, size_t n) {
    int i;
    memset(ram, 0, n);
    for (i = 0; i < 4; ++i)
        wr32(ram + PCB_BASE + (uint32_t)i * PCB_STRIDE, 0u);
    wr32(ram + PCB_NEXT, 2u);
    wr32(ram + 0xA400u, 0u);
    wr32(ram + PCB_BASE + 4u, 1u);
    wr32(ram + PCB_BASE + 0u, 2u);
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

static int count_owned_wins(const haeun_gfx_wm *wm, int pid) {
    int i, n = 0;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i)
        if (wm->windows[i].used && wm->windows[i].owner_pid == pid)
            n++;
    return n;
}

/** EXIT then fault-style second reap; spawn new pid; no stale refs. */
static int test_reuse_and_once(void) {
    haeun_app_env env;
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_evt_hub evt;
    haeun_ipc_hub ipc;
    haeun_svc_hub svc;
    haeun_m20_ecall_ctx m20;
    haeun_ecall_mux_ctx mux;
    static uint8_t ram[2 * 1024 * 1024];
    uint8_t img[256], pay[4] = {9, 8, 7, 6};
    size_t len = 0u;
    uint32_t p_old = 0u, p_new = 0u;
    int w0 = -1, w1 = -1, ep = -1;
    haeun_evt ev;
    int i;
    prep(ram, sizeof(ram));
    memset(&mux, 0, sizeof(mux));
    memset(&m20, 0, sizeof(m20));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, 4u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&evt, &wm);
    haeun_ipc_hub_init(&ipc);
    haeun_svc_hub_init(&svc);
    CHECK(haeun_app_env_init(&env, &vm, &wm) == HAEUN_APP_OK);
    env.evt = &evt;
    make_tiny_hxf(img, &len);
    CHECK(haeun_app_register(&env, "a", img, len) == HAEUN_APP_OK);
    CHECK(haeun_app_spawn(&env, 0, &p_old) == HAEUN_APP_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x20000u, &w0) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 0, 8u, 8u, 0x21000u, &w1) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, w0, (int)p_old) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, w1, (int)p_old) == HAEUN_GFX_OK);
    ep = haeun_ipc_create(&ipc, (int)p_old);
    CHECK(ep >= 0);
    CHECK(haeun_ipc_send(&ipc, ep, (int)p_old, pay, 4u) == 0);
    memset(&ev, 0, sizeof(ev));
    ev.type = (uint16_t)HAEUN_EVT_KEY_DOWN;
    CHECK(haeun_evt_push(&evt, (int)p_old, &ev) == 0);
    CHECK(count_owned_wins(&wm, (int)p_old) == 2);
    m20.evt = &evt;
    m20.ipc = &ipc;
    m20.svc = &svc;
    m20.wm = &wm;
    mux.app_env = &env;
    mux.m20_ctx = &m20;
    wr32(ram + 0xA400u, 1u);
    wr32(ram + PCB_BASE + PCB_STRIDE + 4u, p_old);
    haeun_rf_write(&vm, 1u, HAEUN_SYS_EXIT);
    haeun_rf_write(&vm, 2u, 0u);
    CHECK(!haeun_ecall_mux_io(&vm, &mux));
    CHECK(count_owned_wins(&wm, (int)p_old) == 0);
    CHECK(haeun_evt_poll(&evt, (int)p_old, &ev) != 1);
    CHECK(haeun_ipc_available(&ipc, ep, (int)p_old) < 0);
    CHECK(haeun_app_reap(&env, p_old) == HAEUN_APP_NOENT);
    /* Fault-path second cleanup for same pid must not disturb others. */
    haeun_vm_set_fault_io(&vm, haeun_ecall_mux_on_fault, &mux);
    vm.privilege = HAEUN_PRIV_USER;
    vm.csr_trapvec = 0x100u;
    CHECK(haeun_exception_raise(&vm, HAEUN_VM_MEMORY_FAULT, 5u, 0u) ==
          HAEUN_VM_OK);
    wr32(ram + PCB_BASE + PCB_STRIDE, 0u);
    for (i = 0; i < 6; ++i) {
        uint32_t p = 0u;
        int nw = -1, nep = -1;
        CHECK(haeun_app_spawn(&env, 0, &p) == HAEUN_APP_OK);
        CHECK(p != p_old);
        p_new = p;
        (void)i;
        CHECK(haeun_gfx_wm_create(&wm, 0, 0, 4u, 4u, 0x20000u, &nw) ==
              HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_set_owner(&wm, nw, (int)p) == HAEUN_GFX_OK);
        nep = haeun_ipc_create(&ipc, (int)p);
        CHECK(nep >= 0);
        CHECK(count_owned_wins(&wm, (int)p_old) == 0);
        CHECK(haeun_evt_poll(&evt, (int)p_old, &ev) != 1);
        wr32(ram + 0xA400u, 1u);
        wr32(ram + PCB_BASE + PCB_STRIDE + 4u, p);
        haeun_rf_write(&vm, 1u, HAEUN_SYS_EXIT);
        CHECK(!haeun_ecall_mux_io(&vm, &mux));
        CHECK(count_owned_wins(&wm, (int)p) == 0);
        CHECK(haeun_ipc_available(&ipc, nep, (int)p) < 0);
        wr32(ram + PCB_BASE + PCB_STRIDE, 0u);
    }
    CHECK(p_new != 0u);
    return 0;
}

int main(void) {
    if (test_reuse_and_once() != 0)
        return 1;
    puts("m24_4_resource_reclamation_tests ok");
    return 0;
}
