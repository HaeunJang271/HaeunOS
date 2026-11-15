#include "haeun/vm_input_host.h"

#include <stdio.h>
#include <string.h>

/**
 * Disables KBD/MOUSE IRQ bits so process traps cannot mis-handle sources 3/4.
 */
int haeun_vm_input_host_attach(haeun_vm_input_host *host, haeun_vm *vm,
                               haeun_gfx_wm *wm) {
    if (host == NULL || vm == NULL)
        return -1;
    memset(host, 0, sizeof(*host));
    host->wm = wm;
    vm->kbd_control = 0u;
    vm->mouse_control = 0u;
    host->attached = 1;
    return 0;
}

/**
 * Default script aligned with userland/input_demo.s expectations.
 */
void haeun_vm_input_host_load_demo_script(haeun_vm_input_host *host) {
    if (host == NULL)
        return;
    host->event_count = 0;
    host->next_event = 0;
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 400u, .kind = HAEUN_INPUT_EV_KEY, .scancode = 0x1Cu};
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 420u, .kind = HAEUN_INPUT_EV_KEY, .scancode = 0x9Cu};
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 500u,
        .kind = HAEUN_INPUT_EV_MOUSE,
        .x = 20,
        .y = 4,
        .buttons = 1u};
}

/**
 * Late-timeline script for process apps after SPAWN / WIN_CREATE.
 */
void haeun_vm_input_host_load_m20_script(haeun_vm_input_host *host) {
    if (host == NULL)
        return;
    host->event_count = 0;
    host->next_event = 0;
    /* Client click inside a typical app window at (2,8)-(42,24). */
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 80000u,
        .kind = HAEUN_INPUT_EV_MOUSE,
        .x = 12,
        .y = 12,
        .buttons = 1u};
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 80100u,
        .kind = HAEUN_INPUT_EV_MOUSE,
        .x = 12,
        .y = 12,
        .buttons = 0u};
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 80200u, .kind = HAEUN_INPUT_EV_KEY, .scancode = 0x1Eu};
    host->events[host->event_count++] = (haeun_input_script_ev){
        .at_step = 80250u, .kind = HAEUN_INPUT_EV_KEY, .scancode = 0x9Eu};
}

static void deliver_one(haeun_vm_input_host *host, haeun_vm *vm,
                        const haeun_input_script_ev *ev) {
    int hit = -1;
    uint32_t prev;
    if (ev->kind == HAEUN_INPUT_EV_KEY) {
        uint8_t b = ev->scancode;
        if (haeun_vm_keyboard_feed(vm, &b, 1u))
            host->keys_fed++;
        if (host->chrome != NULL)
            (void)haeun_gfx_chrome_key(host->chrome, b);
        else if (host->evt != NULL) {
            int is_break = (b & 0x80u) != 0u;
            haeun_evt_route_key(host->evt, (uint8_t)(b & 0x7fu), is_break);
        }
        return;
    }
    if (ev->kind == HAEUN_INPUT_EV_MOUSE) {
        if (!haeun_vm_mouse_post(vm, ev->x, ev->y, ev->buttons))
            return;
        host->mice_posted++;
        prev = host->prev_buttons;
        host->prev_buttons = ev->buttons & 7u;
        if (host->chrome != NULL) {
            int prev_focus =
                host->chrome->wm != NULL ? host->chrome->wm->focus_id : -1;
            (void)haeun_gfx_chrome_pointer(host->chrome, ev->x, ev->y,
                                           ev->buttons);
            if (host->chrome->wm != NULL && host->chrome->wm->focus_id >= 0 &&
                host->chrome->wm->focus_id != prev_focus)
                host->focus_hits++;
            return;
        }
        if (host->wm != NULL && (ev->buttons & 1u) != 0u) {
            if (haeun_gfx_wm_hit_focus(host->wm, ev->x, ev->y, &hit) ==
                    HAEUN_GFX_OK &&
                hit >= 0)
                host->focus_hits++;
        }
        if (host->evt != NULL)
            haeun_evt_route_mouse(host->evt, ev->x, ev->y, ev->buttons, prev);
    }
}

/**
 * Runs until halt/fault/budget while applying the scripted event timeline.
 */
haeun_vm_result haeun_vm_input_host_run(haeun_vm_input_host *host, haeun_vm *vm,
                                        uint64_t max_steps,
                                        uint64_t *steps_executed) {
    if (host == NULL || vm == NULL || steps_executed == NULL)
        return HAEUN_VM_BAD_ARGUMENT;
    *steps_executed = 0u;
    while (*steps_executed < max_steps) {
        haeun_vm_result st;
        while (host->next_event < host->event_count &&
               host->events[host->next_event].at_step <= *steps_executed) {
            deliver_one(host, vm, &host->events[host->next_event]);
            host->next_event++;
        }
        st = haeun_vm_step(vm);
        ++(*steps_executed);
        if (st == HAEUN_VM_HALTED)
            return st;
        if (st != HAEUN_VM_OK)
            return st;
    }
    return HAEUN_VM_STEP_LIMIT;
}
