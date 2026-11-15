/**
 * M20.1: per-PID queues, focus routing, overflow, lifecycle, EVENT_POLL ABI.
 */
#include "haeun/evt.h"
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

static int test_fifo_and_overflow(void) {
    haeun_evt_hub hub;
    haeun_evt ev, out;
    int i;
    haeun_evt_hub_init(&hub, NULL);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAEUN_EVT_KEY_DOWN;
    for (i = 0; i < 5; ++i) {
        ev.a = (uint32_t)i;
        CHECK(haeun_evt_push(&hub, 7, &ev) == 0);
    }
    for (i = 0; i < 5; ++i) {
        CHECK(haeun_evt_poll(&hub, 7, &out) == 1);
        CHECK(out.a == (uint32_t)i);
    }
    CHECK(haeun_evt_poll(&hub, 7, &out) == 0);
    for (i = 0; i < HAEUN_EVT_QUEUE + 2; ++i) {
        ev.a = (uint32_t)(100 + i);
        CHECK(haeun_evt_push(&hub, 7, &ev) == 0);
    }
    CHECK(hub.slots[0].drops == 2u);
    CHECK(haeun_evt_poll(&hub, 7, &out) == 1);
    CHECK(out.a == 102u);
    return 0;
}

static int test_focus_routing(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    haeun_evt_hub hub;
    haeun_evt out;
    int id_a = -1, id_b = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 16u, 16u, 0x20000u, &id_a) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 20, 0, 16u, 16u, 0x22000u, &id_b) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id_a, 2);
    (void)haeun_gfx_wm_set_owner(&wm, id_b, 3);
    (void)haeun_gfx_wm_set_focus(&wm, id_a);
    haeun_evt_on_focus(&hub, id_a);
    haeun_evt_route_key(&hub, 0x1Eu, 0);
    CHECK(haeun_evt_poll(&hub, 2, &out) == 1);
    CHECK(out.type == HAEUN_EVT_FOCUS || out.type == HAEUN_EVT_KEY_DOWN);
    if (out.type == HAEUN_EVT_FOCUS) {
        CHECK(haeun_evt_poll(&hub, 2, &out) == 1);
        CHECK(out.type == HAEUN_EVT_KEY_DOWN);
    }
    CHECK(out.a == 0x1Eu);
    CHECK(haeun_evt_poll(&hub, 3, &out) == 0);
    (void)haeun_gfx_wm_set_focus(&wm, id_b);
    haeun_evt_on_focus(&hub, id_b);
    haeun_evt_route_key(&hub, 0x2Eu, 0);
    CHECK(haeun_evt_poll(&hub, 3, &out) == 1);
    if (out.type == HAEUN_EVT_FOCUS)
        CHECK(haeun_evt_poll(&hub, 3, &out) == 1);
    CHECK(out.type == HAEUN_EVT_KEY_DOWN && out.a == 0x2Eu);
    CHECK(haeun_evt_poll(&hub, 2, &out) == 1);
    CHECK(out.type == HAEUN_EVT_BLUR);
    return 0;
}

static int test_mouse_and_wheel(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    haeun_evt_hub hub;
    haeun_evt out;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);
    CHECK(haeun_gfx_wm_create(&wm, 4, 4, 20u, 12u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id, 5);
    (void)haeun_gfx_wm_set_focus(&wm, id);
    haeun_evt_route_mouse(&hub, 10, 8, 1u, 0u);
    CHECK(haeun_evt_poll(&hub, 5, &out) == 1);
    CHECK(out.type == HAEUN_EVT_MOUSE_DOWN);
    CHECK(out.a == 6u && out.b == 4u);
    haeun_evt_route_mouse(&hub, 11, 8, 1u, 1u);
    CHECK(haeun_evt_poll(&hub, 5, &out) == 1);
    CHECK(out.type == HAEUN_EVT_MOUSE_MOVE);
    haeun_evt_route_wheel(&hub, 10, 8, -120);
    CHECK(haeun_evt_poll(&hub, 5, &out) == 1);
    CHECK(out.type == HAEUN_EVT_MOUSE_WHEEL);
    CHECK((int32_t)out.b == -120);
    return 0;
}

static int test_reap_reuse_destroy(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    haeun_evt_hub hub;
    haeun_evt ev, out;
    int id = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);
    memset(&ev, 0, sizeof(ev));
    ev.type = HAEUN_EVT_KEY_DOWN;
    ev.a = 0xAAu;
    CHECK(haeun_evt_push(&hub, 9, &ev) == 0);
    haeun_evt_reap(&hub, 9);
    CHECK(haeun_evt_poll(&hub, 9, &out) == 0);
    CHECK(haeun_evt_push(&hub, 9, &ev) == 0);
    CHECK(haeun_evt_poll(&hub, 9, &out) == 1);
    CHECK(out.a == 0xAAu);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, 0x20000u, &id) ==
          HAEUN_GFX_OK);
    (void)haeun_gfx_wm_set_owner(&wm, id, 9);
    (void)haeun_gfx_wm_set_focus(&wm, id);
    haeun_evt_on_focus(&hub, id);
    (void)haeun_gfx_wm_destroy(&wm, id);
    haeun_evt_sync_focus(&hub);
    haeun_evt_route_key(&hub, 0x1Cu, 0);
    CHECK(haeun_evt_poll(&hub, 9, &out) == 1);
    CHECK(out.type == HAEUN_EVT_FOCUS || out.type == HAEUN_EVT_BLUR);
    while (haeun_evt_poll(&hub, 9, &out)) {
        CHECK(out.type != HAEUN_EVT_KEY_DOWN);
    }
    return 0;
}

static int test_event_poll_ecall(void) {
    uint8_t ram[0x40000];
    haeun_vm vm;
    haeun_evt_hub hub;
    haeun_m20_ecall_ctx ctx;
    haeun_evt ev;
    memset(ram, 0, sizeof(ram));
    put_le32(ram + 0u, ENC(0x0d, 0, 0, 0, 0)); /* ECALL */
    CHECK(haeun_vm_init_version(&vm, ram, sizeof(ram), 0u, HAEUN_ISA_VERSION_4) ==
          HAEUN_VM_OK);
    haeun_evt_hub_init(&hub, NULL);
    memset(&ctx, 0, sizeof(ctx));
    ctx.evt = &hub;
    haeun_vm_set_ecall_io(&vm, haeun_m20_ecall_io, &ctx);
    set_pcb_pid(ram, 1u, 4u);
    vm.privilege = HAEUN_PRIV_USER;
    memset(&ev, 0, sizeof(ev));
    ev.type = HAEUN_EVT_KEY_DOWN;
    ev.win = 1;
    ev.a = 0x1Eu;
    CHECK(haeun_evt_push(&hub, 4, &ev) == 0);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_EVENT_POLL;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == HAEUN_EVT_KEY_DOWN);
    CHECK(vm.r[2] == 1u);
    CHECK(vm.r[3] == 0x1Eu);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_EVENT_POLL;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    set_pcb_pid(ram, 1u, 0u);
    vm.pc = 0u;
    vm.r[1] = HAEUN_SYS_EVENT_POLL;
    CHECK(haeun_vm_step(&vm) == HAEUN_VM_OK);
    CHECK(vm.r[1] == 0u);
    return 0;
}

int main(void) {
    if (test_fifo_and_overflow() != 0)
        return 1;
    if (test_focus_routing() != 0)
        return 1;
    if (test_mouse_and_wheel() != 0)
        return 1;
    if (test_reap_reuse_destroy() != 0)
        return 1;
    if (test_event_poll_ecall() != 0)
        return 1;
    (void)printf("m20_input_tests ok\n");
    return 0;
}
