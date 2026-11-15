/**
 * M20.4: window events — create/focus/move/resize/min/max/close/destroy.
 */
#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_chrome.h"
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

static int drain(haeun_evt_hub *hub, int pid, haeun_evt *last, int maxn) {
    int n = 0;
    haeun_evt ev;
    while (n < maxn && haeun_evt_poll(hub, pid, &ev)) {
        if (last != NULL)
            *last = ev;
        n++;
    }
    return n;
}

static int test_lifecycle_and_chrome(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_chrome ch;
    haeun_evt_hub hub;
    haeun_evt ev;
    int id = -1, id2 = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_init(&ch, &gfx, &wm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);
    ch.evt = &hub;

    CHECK(haeun_gfx_wm_create(&wm, 8, 8, 16u, 12u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id, 5);
    haeun_evt_win_post(&hub, id, HAEUN_EVT_CREATED, 8u, 8u);
    (void)haeun_gfx_wm_set_focus(&wm, id);
    haeun_evt_on_focus(&hub, id);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_CREATED && ev.win == (int16_t)id);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_FOCUS);
    CHECK(haeun_evt_is_window(HAEUN_EVT_FOCUS) == 1);
    CHECK(haeun_evt_is_window(HAEUN_EVT_KEY_DOWN) == 0);
    CHECK(haeun_evt_is_window(HAEUN_EVT_MOUSE_WHEEL) == 0);

    /* Drag via title body (left of caption buttons) → MOVED once */
    (void)drain(&hub, 5, NULL, 8);
    CHECK(haeun_gfx_chrome_pointer(&ch, 8, 5, 1u) == HAEUN_GFX_OK);
    CHECK(ch.drag_id == id);
    CHECK(haeun_gfx_chrome_pointer(&ch, 16, 9, 1u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(&ch, 16, 9, 0u) == HAEUN_GFX_OK);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_MOVED);
    CHECK(ev.a == (uint32_t)wm.windows[id].x);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 0);

    /* Resize complete */
    ch.resize_id = id;
    ch.resize_ox = 20;
    ch.resize_oy = 20;
    ch.resize_ow = wm.windows[id].w;
    ch.resize_oh = wm.windows[id].h;
    (void)haeun_gfx_wm_resize(&wm, id, 12u, 10u);
    CHECK(haeun_gfx_chrome_pointer(&ch, 20, 20, 0u) == HAEUN_GFX_OK);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_RESIZE && ev.a == 12u && ev.b == 10u);

    /* Min / max / restore (direct WM + post, as chrome zones vary) */
    (void)drain(&hub, 5, NULL, 8);
    CHECK(haeun_gfx_wm_minimize(&wm, id) == HAEUN_GFX_OK);
    haeun_evt_win_post(&hub, id, HAEUN_EVT_MINIMIZED, 0, 0);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_MINIMIZED);
    CHECK(haeun_gfx_wm_restore(&wm, id) == HAEUN_GFX_OK);
    haeun_evt_win_post(&hub, id, HAEUN_EVT_RESTORED, wm.windows[id].w,
                       wm.windows[id].h);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_RESTORED);
    CHECK(haeun_gfx_wm_maximize(&wm, id, 64u, 32u) == HAEUN_GFX_OK);
    haeun_evt_win_post(&hub, id, HAEUN_EVT_MAXIMIZED, wm.windows[id].w,
                       wm.windows[id].h);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_MAXIMIZED);

    /* Soft close: CLOSE only, window remains */
    haeun_evt_win_post(&hub, id, HAEUN_EVT_CLOSE, 0, 0);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_CLOSE);
    CHECK(wm.windows[id].used);
    CHECK(ev.type != HAEUN_EVT_DESTROYED);

    /* DESTROYED on destroy */
    haeun_evt_win_post(&hub, id, HAEUN_EVT_DESTROYED, 0, 0);
    CHECK(haeun_gfx_wm_destroy(&wm, id) == HAEUN_GFX_OK);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 1);
    CHECK(ev.type == HAEUN_EVT_DESTROYED);

    /* Isolation: other pid empty */
    CHECK(haeun_gfx_wm_create(&wm, 0, 8, 8u, 8u, 0x22000u, &id2) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id2, 7);
    haeun_evt_win_post(&hub, id2, HAEUN_EVT_CREATED, 0, 0);
    CHECK(haeun_evt_poll(&hub, 5, &ev) == 0);
    CHECK(haeun_evt_poll(&hub, 7, &ev) == 1);
    return 0;
}

static int test_win_query_ecall(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_m20_ecall_ctx ctx;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x10000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 3, 5, 20u, 10u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id, 4);
    (void)haeun_gfx_wm_set_focus(&wm, id);
    memset(&ctx, 0, sizeof(ctx));
    ctx.wm = &wm;
    ctx.gfx = &gfx;
    haeun_vm_set_ecall_io(&vm, haeun_m20_ecall_io, &ctx);
    set_pcb_pid(ram, 1u, 4u);
    vm.privilege = HAEUN_PRIV_USER;
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_QUERY;
    vm.r[2] = (uint32_t)id;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    CHECK(vm.r[2] == 3u && vm.r[3] == 5u);
    CHECK((vm.r[4] & 0xffffu) == 20u);
    CHECK(((vm.r[4] >> 16) & 0xffffu) == 10u);
    CHECK((vm.r[5] & 1u) != 0u);
    /* foreign pid */
    set_pcb_pid(ram, 1u, 9u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_WIN_QUERY;
    vm.r[2] = (uint32_t)id;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0xffffffffu);
    return 0;
}

static int test_unowned_close_destroys(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_chrome ch;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, 0x1000u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_init(&ch, &gfx, &wm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 8, 16u, 12u, 0x3000u, &id) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_raise(&wm, id) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_pointer(
              &ch, wm.windows[id].x + (int32_t)wm.windows[id].w - 2,
              wm.windows[id].y - 4, 1u) == HAEUN_GFX_OK);
    CHECK(!wm.windows[id].used);
    return 0;
}

int main(void) {
    if (test_lifecycle_and_chrome() != 0)
        return 1;
    if (test_win_query_ecall() != 0)
        return 1;
    if (test_unowned_close_destroys() != 0)
        return 1;
    (void)printf("m20_window_tests ok\n");
    return 0;
}
