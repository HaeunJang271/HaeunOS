/**
 * M23.4: measurable WM resource/composition/input-stress stabilization.
 * Prints slot/focus/evt counters; asserts reclamation invariants.
 */
#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_chrome.h"
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

enum { FB = 0x1000u };

static int count_used(const haeun_gfx_wm *wm) {
    int i, n = 0;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i)
        if (wm->windows[i].used)
            n++;
    return n;
}

static int count_owned(const haeun_gfx_wm *wm, int pid) {
    int i, n = 0;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i)
        if (wm->windows[i].used && wm->windows[i].owner_pid == pid)
            n++;
    return n;
}

static int focus_ok(const haeun_gfx_wm *wm) {
    int id = wm->focus_id;
    if (id < 0)
        return 1;
    if (id >= HAEUN_GFX_WM_MAX_WINDOWS)
        return 0;
    return wm->windows[id].used && wm->windows[id].visible &&
           !wm->windows[id].minimized;
}

static uint32_t evt_queued(const haeun_evt_hub *hub) {
    int i;
    uint32_t n = 0u;
    for (i = 0; i < HAEUN_EVT_SLOTS; ++i)
        n += hub->slots[i].count;
    return n;
}

/** Scenario A: multi-window raise/key/min/restore/destroy/compose. */
static int test_scenario_a(void) {
    uint8_t ram[0x20000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_evt_hub hub;
    haeun_evt out;
    int a = -1, b = -1, c = -1;
    uint32_t px = 0u;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, FB) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);

    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 20u, 16u, 0x3000u, &a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 4, 20u, 16u, 0x4000u, &b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 4, 8, 12u, 12u, 0x5000u, &c) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, a, 2) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, b, 3) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, c, 2) == HAEUN_GFX_OK);
    CHECK(count_owned(&wm, 2) == 2);

    CHECK(haeun_gfx_wm_fill(&wm, a, 0x000000FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, b, 0x0000FF00u) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_fill(&wm, c, 0x00FF0000u) == HAEUN_GFX_OK);

    CHECK(haeun_gfx_wm_raise(&wm, a) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, a) == HAEUN_GFX_OK);
    haeun_evt_route_key(&hub, 0x1Eu, 0);
    CHECK(haeun_evt_poll(&hub, 2, &out) == 1);
    CHECK(out.type == HAEUN_EVT_KEY_DOWN && out.a == 0x1Eu);
    CHECK(haeun_evt_poll(&hub, 3, &out) == 0);

    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_OK);
    haeun_evt_route_key(&hub, 0x30u, 0);
    CHECK(haeun_evt_poll(&hub, 3, &out) == 1);
    CHECK(out.a == 0x30u);
    CHECK(haeun_evt_poll(&hub, 2, &out) == 0);

    CHECK(haeun_gfx_wm_minimize(&wm, b) == HAEUN_GFX_OK);
    CHECK(focus_ok(&wm));
    CHECK(wm.focus_id != b);
    CHECK(haeun_gfx_compose(&gfx, &wm, 0x010101FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 12u, 8u, &px) == HAEUN_GFX_OK);
    CHECK(px != 0x0000FF00u); /* minimized b not blitted */

    CHECK(haeun_gfx_wm_restore(&wm, b) == HAEUN_GFX_OK);
    CHECK(!wm.windows[b].minimized && wm.windows[b].visible);
    CHECK(haeun_gfx_wm_raise(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_focus(&wm, b) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_compose(&gfx, &wm, 0x010101FFu) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_get_pixel(&gfx, 12u, 8u, &px) == HAEUN_GFX_OK);
    CHECK(px == 0x0000FF00u);

    CHECK(haeun_gfx_wm_destroy(&wm, c) == HAEUN_GFX_OK);
    CHECK(!wm.windows[c].used);
    CHECK(wm.windows[c].owner_pid == 0);
    CHECK(count_owned(&wm, 2) == 1);
    CHECK(focus_ok(&wm));
    CHECK(haeun_gfx_compose(&gfx, &wm, 0x010101FFu) == HAEUN_GFX_OK);
    (void)printf("m23.4-A: used=%d focus=%d queued=%u\n", count_used(&wm),
                 wm.focus_id, evt_queued(&hub));
    return 0;
}

/** Scenario C + resource counters: capacity, reclaim, focus stress. */
static int test_scenario_c_resources(void) {
    uint8_t ram[0x20000];
    haeun_vm vm;
    haeun_gfx_wm wm;
    haeun_evt_hub hub;
    haeun_evt out;
    int ids[8];
    int i, extra = -1;
    int used_before, used_after;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    haeun_evt_hub_init(&hub, &wm);

    for (i = 0; i < 8; ++i) {
        ids[i] = -1;
        CHECK(haeun_gfx_wm_create(&wm, (int32_t)(i * 4), 0, 4u, 4u,
                                  (uint32_t)(0x3000u + (uint32_t)i * 0x100u),
                                  &ids[i]) == HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_set_owner(&wm, ids[i], 5) == HAEUN_GFX_OK);
        CHECK(haeun_gfx_wm_set_focus(&wm, ids[i]) == HAEUN_GFX_OK);
        haeun_evt_route_key(&hub, 0x02u, 0);
        while (haeun_evt_poll(&hub, 5, &out)) {
        }
    }
    used_before = count_used(&wm);
    CHECK(used_before == 8);
    CHECK(haeun_gfx_wm_create(&wm, 0, 8, 4u, 4u, 0x3800u, &extra) ==
          HAEUN_GFX_BAD_ARGUMENT);

    for (i = 0; i < 24; ++i) {
        int id = ids[i % 8];
        CHECK(haeun_gfx_wm_raise(&wm, id) == HAEUN_GFX_OK);
        if ((i % 3) == 0) {
            CHECK(haeun_gfx_wm_minimize(&wm, id) == HAEUN_GFX_OK);
            CHECK(focus_ok(&wm));
            CHECK(haeun_gfx_wm_restore(&wm, id) == HAEUN_GFX_OK);
        }
        if ((i % 5) == 0) {
            CHECK(haeun_gfx_wm_destroy(&wm, id) == HAEUN_GFX_OK);
            CHECK(!wm.windows[id].used);
            CHECK(wm.windows[id].owner_pid == 0);
            CHECK(haeun_gfx_wm_create(
                      &wm, (int32_t)((i % 8) * 4), 0, 4u, 4u,
                      (uint32_t)(0x3000u + (uint32_t)(i % 8) * 0x100u),
                      &ids[i % 8]) == HAEUN_GFX_OK);
            CHECK(haeun_gfx_wm_set_owner(&wm, ids[i % 8], 5) == HAEUN_GFX_OK);
        }
        CHECK(haeun_gfx_wm_set_focus(&wm, ids[i % 8]) == HAEUN_GFX_OK);
        haeun_evt_route_key(&hub, 0x03u, 0);
        /* Drain so stress measures WM reclaim, not evt queue overflow. */
        while (haeun_evt_poll(&hub, 5, &out)) {
        }
        CHECK(focus_ok(&wm));
    }

    /* Owner exit: destroy all pid=5 windows. */
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        if (wm.windows[i].used && wm.windows[i].owner_pid == 5)
            CHECK(haeun_gfx_wm_destroy(&wm, i) == HAEUN_GFX_OK);
    }
    used_after = count_used(&wm);
    CHECK(count_owned(&wm, 5) == 0);
    CHECK(focus_ok(&wm));
    haeun_evt_reap(&hub, 5);
    CHECK(evt_queued(&hub) == 0u);

    (void)printf("m23.4-C: used_before=%d used_after=%d next_z=%u focus=%d "
                 "queued=%u\n",
                 used_before, used_after, wm.next_z, wm.focus_id,
                 evt_queued(&hub));
    CHECK(used_after == 0);
    return 0;
}

/** Hit-test prefers owned app over fullscreen unowned shell. */
static int test_shell_hit(void) {
    uint8_t ram[0x10000];
    haeun_vm vm;
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_chrome ch;
    int shell = -1, app = -1, hit = -1;
    memset(ram, 0, sizeof(ram));
    CHECK(haeun_vm_init(&vm, ram, sizeof(ram), 0u) == HAEUN_VM_OK);
    CHECK(haeun_gfx_fb_enable(&gfx, &vm, 64u, 32u, FB) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_init(&wm, &vm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_chrome_init(&ch, &gfx, &wm) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 0, 0, 64u, 32u, 0x3000u, &shell) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_create(&wm, 8, 8, 16u, 12u, 0x8000u, &app) ==
          HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_set_owner(&wm, app, 7) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_raise(&wm, shell) == HAEUN_GFX_OK);
    CHECK(haeun_gfx_wm_hit_focus(&wm, 10, 10, &hit) == HAEUN_GFX_OK);
    CHECK(hit == app);
    CHECK(haeun_gfx_chrome_pointer(&ch, 10, 10, 1u) == HAEUN_GFX_OK);
    CHECK(wm.focus_id == app);
    return 0;
}

int main(void) {
    if (test_scenario_a() != 0)
        return 1;
    if (test_scenario_c_resources() != 0)
        return 1;
    if (test_shell_hit() != 0)
        return 1;
    puts("m23.4 resource/stabilization tests passed");
    return 0;
}
