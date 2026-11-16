/**
 * M23.3: window state transitions, eligibility, owner exit, capacity stress.
 */
#include "haeun/evt.h"
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

enum { FB = 0x1000u, A = 0x3000u, B = 0x4000u, C = 0x5000u, D = 0x6000u };

static int count_used(const haeun_gfx_wm *wm) {
    int i, n = 0;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i)
        if (wm->windows[i].used)
            n++;
    return n;
}

/** Create/show/hide/min/restore/raise/focus/destroy + invalid combos. */
static int test_state_transitions(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    int a = -1, b = -1;
    uint32_t px = 0u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, FB) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == -1);

    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 16u, 12u, A, &a) == HAEUN_GFX_OK);
    CHECK(wm.windows[a].used && wm.windows[a].visible && !wm.windows[a].minimized);
    CHECK(haeun_gfx_wm_set_owner(&wm, a, 2) == HAEUN_GFX_OK);
    CHECK(wm.windows[a].owner_pid == 2);
    CHECK(haeun_gfx_wm_set_focus(&wm, a) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == a);

    CHECK(haeun_gfx_wm_create(&wm, 8, 4, 16u, 12u, B, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, b, 3) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(wm.windows[b].z > wm.windows[a].z);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_OK);

    CHECK(haeun_gfx_wm_minimize(&wm, b) == HAEUN_GFX_OK);
    CHECK(wm.windows[b].minimized && !wm.windows[b].visible);
    CHECK(wm.focus_id == a);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_BAD_ARGUMENT);
    CHECK(wm.focus_id == a);

    CHECK(haeun_gfx_wm_fill(&wm, a, 0x000000FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, b, 0x0000FF00u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, 0x010101FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 2u, 2u, &px) == HAEUN_GFX_OK);
    CHECK(px == 0x000000FFu); /* a visible; b minimized not blitted */

    CHECK(haeun_gfx_wm_restore(&wm, b) == HAEUN_GFX_OK);
    CHECK(!wm.windows[b].minimized && wm.windows[b].visible);
    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, 0x010101FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 10u, 6u, &px) == HAEUN_GFX_OK);
    CHECK(px == 0x0000FF00u); /* overlap: b on top */

    CHECK(haeun_gfx_wm_set_visible(&wm, b, 0) == HAEUN_GFX_OK);
    CHECK(!wm.windows[b].visible);
    CHECK(wm.focus_id == a);
    CHECK(haeun_gfx_wm_set_visible(&wm, b, 1) == HAEUN_GFX_OK);
    CHECK(wm.windows[b].visible && !wm.windows[b].minimized);

    /* minimize then set_visible(1) must clear minimized */
    CHECK(haeun_gfx_wm_minimize(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_visible(&wm, b, 1) == HAEUN_GFX_OK);
    CHECK(!wm.windows[b].minimized && wm.windows[b].visible);

    CHECK(haeun_gfx_wm_destroy(&wm, b) == HAEUN_GFX_OK);
    CHECK(!wm.windows[b].used);
    CHECK(wm.focus_id == a);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_BAD_ARGUMENT);
    return 0;
}

/** Owner exit destroys all owned windows; focus recovers; no orphans. */
static int test_owner_exit_multi(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int a = -1, b = -1, c = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, A, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 0, 8u, 8u, B, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 16, 0, 8u, 8u, C, &c) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, a, 9) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, b, 9) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, c, 4) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_OK);
    /* Simulate owner pid=9 exit: destroy every owned window. */
    {
        int i;
        for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
            if (wm.windows[i].used && wm.windows[i].owner_pid == 9)
                CHECK(haeun_gfx_wm_destroy(&wm, i) == HAEUN_GFX_OK);
        }
    }
    CHECK(!wm.windows[a].used && !wm.windows[b].used);
    CHECK(wm.windows[c].used && wm.windows[c].owner_pid == 4);
    CHECK(wm.focus_id == c);
    return 0;
}

/** Capacity 8, exhaust, free, recreate; focus never stale. */
static int test_capacity_and_stress(void) {
    uint8_t ram[0x20000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    int ids[8];
    int i, extra = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    for (i = 0; i < 8; ++i) {
        ids[i] = -1;
        CHECK(haeun_gfx_wm_create(&wm, (int32_t)(i * 4), 0, 4u, 4u,
                                  (uint32_t)(0x3000u + (uint32_t)i * 0x100u),
                                  &ids[i]) == HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_set_focus(&wm, ids[i]) == HAEUN_GFX_OK);
    }
    CHECK(count_used(&wm) == 8);
    CHECK(haeun_gfx_wm_create(&wm, 0, 8, 4u, 4u, 0x3800u, &extra) ==
          HAEUN_GFX_BAD_ARGUMENT);
    for (i = 0; i < 16; ++i) {
        CHECK(haeun_gfx_wm_destroy(&wm, ids[i % 8]) == HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_create(&wm, (int32_t)((i % 8) * 4), 0, 4u, 4u,
                                  (uint32_t)(0x3000u + (uint32_t)(i % 8) * 0x100u),
                                  &ids[i % 8]) == HAEUN_GFX_OK);
        if (wm.focus_id >= 0) {
            CHECK(wm.windows[wm.focus_id].used);
            CHECK(wm.windows[wm.focus_id].visible);
            CHECK(!wm.windows[wm.focus_id].minimized);
        }
    }
    CHECK(count_used(&wm) == 8);
    return 0;
}

/** Keys must not route to minimized/hidden focus. */
static int test_key_eligibility(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    haeun_evt_hub hub;
    haeun_evt out;
    int a = -1, b = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 8u, 8u, A, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 0, 8u, 8u, B, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, a, 2) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, b, 3) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, a) == HAEUN_GFX_OK);
    /* Force stale minimized focus (bypass set_focus) then route must drop. */
    wm.windows[a].minimized = 1;
    wm.windows[a].visible = 0;
    wm.focus_id = a;
    haeun_evt_route_key(&hub, 0x1Eu, 0);
    CHECK(haeun_evt_poll(&hub, 2, &out) == 0);
    CHECK(haeun_evt_poll(&hub, 3, &out) == 0);
    (void)haeun_gfx_wm_focus_recover(&wm);
    CHECK(wm.focus_id == b);
    haeun_evt_route_key(&hub, 0x30u, 0);
    CHECK(haeun_evt_poll(&hub, 3, &out) == 1);
    CHECK(out.type == HAEUN_EVT_KEY_DOWN && out.a == 0x30u);
    CHECK(haeun_evt_poll(&hub, 2, &out) == 0);
    return 0;
}

int main(void) {
    if (test_state_transitions() != 0)
        return 1;
    if (test_owner_exit_multi() != 0)
        return 1;
    if (test_capacity_and_stress() != 0)
        return 1;
    if (test_key_eligibility() != 0)
        return 1;
    puts("m23.3 window state tests passed");
    return 0;
}
