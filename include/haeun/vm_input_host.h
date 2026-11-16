#ifndef HAEUN_VM_INPUT_HOST_H
#define HAEUN_VM_INPUT_HOST_H

#include "haeun/evt.h"
#include "haeun/gfx_chrome.h"
#include "haeun/gfx_wm.h"
#include "haeun/vm.h"
#include "haeun/vm_gui_host.h"

#include <stdint.h>

enum { HAEUN_INPUT_HOST_MAX_EVENTS = 64 };

typedef enum {
    HAEUN_INPUT_EV_KEY = 1,
    HAEUN_INPUT_EV_MOUSE = 2
} haeun_input_ev_kind;

typedef struct {
    uint64_t at_step;
    haeun_input_ev_kind kind;
    uint8_t scancode;
    int32_t x;
    int32_t y;
    uint32_t buttons;
} haeun_input_script_ev;

typedef struct {
    haeun_gfx_wm *wm; /* optional; enables click→focus routing */
    haeun_gfx_chrome *chrome; /* optional M12; routes into evt hub */
    haeun_evt_hub *evt; /* optional direct route when chrome is NULL */
    haeun_input_script_ev events[HAEUN_INPUT_HOST_MAX_EVENTS];
    int event_count;
    int next_event;
    int keys_fed;
    int mice_posted;
    int focus_hits;
    uint32_t prev_buttons;
    int attached;
} haeun_vm_input_host;

/**
 * Arms scripted input; keeps KBD/MOUSE IRQ disabled (poll-only).
 * wm may be NULL, or &gui_host->wm after GUI attach.
 */
int haeun_vm_input_host_attach(haeun_vm_input_host *host, haeun_vm *vm,
                               haeun_gfx_wm *wm);

/** Loads the default M10 demo script (make+break + click on green window). */
void haeun_vm_input_host_load_demo_script(haeun_vm_input_host *host);

/**
 * Late-timeline script for process apps (after SPAWN / WIN_CREATE).
 * Click then key make/break so EVENT_POLL can observe focus-routed events.
 */
void haeun_vm_input_host_load_m20_script(haeun_vm_input_host *host);

/**
 * M21 persistent-desktop script: idle window, spawn keys, NOSPC, Esc shutdown.
 * Does not fabricate guest queue events beyond the existing input-host path.
 */
void haeun_vm_input_host_load_m21_script(haeun_vm_input_host *host);

/**
 * M21.3 stress script: events after the historical 10M budget, then Esc.
 * Used to prove persist mode survives past INSTRUCTION_BUDGET.
 */
void haeun_vm_input_host_load_m21_stress_script(haeun_vm_input_host *host);

/**
 * M22.3 interactive scripts (guest EVENT_POLL path via chrome/evt).
 * Names: calc-add, calc-sub, calc-mul, calc-div, calc-bs, calc-esc,
 * edit-type, files-list, life-cycle. Returns 0 on success, -1 unknown.
 */
int haeun_vm_input_host_load_m22_3_script(haeun_vm_input_host *host,
                                          const char *name);

/**
 * Steps the guest, injecting scripted events at at_step thresholds.
 * Left-button posts also call haeun_gfx_wm_hit_focus when wm is set.
 */
haeun_vm_result haeun_vm_input_host_run(haeun_vm_input_host *host, haeun_vm *vm,
                                        uint64_t max_steps,
                                        uint64_t *steps_executed);

#endif
