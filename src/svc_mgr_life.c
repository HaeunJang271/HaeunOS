#include "haeun/svc_mgr.h"

#include <stdio.h>
#include <string.h>

enum {
    PCB_BASE = 0x0000A000u,
    PCB_STRIDE = 0x100u,
    PCB_NPROC = 4,
    STATE_FREE = 0,
    STATE_READY = 1,
    STATE_RUNNING = 2,
    STATE_ZOMBIE = 5
};

static void sm_log(const char *m) {
    fprintf(stderr, "[haeun-sm] %s\n", m != NULL ? m : "");
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int pcb_lookup(const haeun_vm *vm, uint32_t pid, uint32_t *state,
                      uint32_t *exit_code) {
    int i;
    if (vm == NULL || vm->ram == NULL || pid == 0u)
        return -1;
    for (i = 0; i < PCB_NPROC; ++i) {
        uint32_t off = PCB_BASE + (uint32_t)i * PCB_STRIDE;
        if ((uint64_t)off + 20u > vm->ram_size)
            return -1;
        if (rd32(vm->ram + off + 4u) == pid) {
            if (state != NULL)
                *state = rd32(vm->ram + off);
            if (exit_code != NULL)
                *exit_code = rd32(vm->ram + off + 16u);
            return i;
        }
    }
    return -1;
}

static int live_pcb(uint32_t st) {
    return st == STATE_READY || st == STATE_RUNNING || st == 3u || st == 4u;
}

static void try_pending(haeun_sm_hub *hub, haeun_sm_entry *e) {
    haeun_sm_result r;
    if (hub == NULL || e == NULL || !e->pending_restart)
        return;
    if (e->restart_count >= e->max_restarts) {
        e->pending_restart = 0u;
        sm_log("restart limit reached");
        return;
    }
    if (e->state == HAEUN_SM_RUNNING || e->state == HAEUN_SM_STARTING ||
        e->state == HAEUN_SM_STOPPING)
        return;
    e->restart_count++;
    e->pending_restart = 0u;
    r = haeun_sm_start(hub, (int)(e - hub->tab));
    if (r != HAEUN_SM_OK) {
        e->pending_restart = 1u; /* retry on next status */
        sm_log("restart spawn deferred");
    }
}

/**
 * Syncs with PCB and fills status (real process state).
 */
haeun_sm_result haeun_sm_status(haeun_sm_hub *hub, int id, haeun_sm_stat *out) {
    haeun_sm_entry *e;
    uint32_t st = 0u;
    uint32_t code = 0u;
    int pcb;
    if (hub == NULL || id < 0 || id >= hub->count || !hub->tab[id].used)
        return HAEUN_SM_NOENT;
    e = &hub->tab[id];
    if (e->pid != 0u) {
        pcb = pcb_lookup(hub->vm, e->pid, &st, &code);
        if (pcb < 0 || st == STATE_FREE) {
            if (e->state == HAEUN_SM_STOPPING) {
                e->state = HAEUN_SM_STOPPED;
                e->last_exit = code;
            } else if (e->state == HAEUN_SM_RUNNING ||
                       e->state == HAEUN_SM_STARTING) {
                e->state = HAEUN_SM_FAILED;
                e->last_exit = code;
                e->fail_count++;
            }
            e->pid = 0u;
        } else if (st == STATE_ZOMBIE) {
            e->last_exit = code;
            if (e->state == HAEUN_SM_STOPPING)
                e->state = HAEUN_SM_STOPPED;
            else if (e->state == HAEUN_SM_RUNNING)
                e->state = HAEUN_SM_FAILED;
        } else if (live_pcb(st)) {
            if (e->state == HAEUN_SM_STARTING)
                e->state = HAEUN_SM_RUNNING;
        }
    }
    try_pending(hub, e);
    if (out != NULL) {
        out->state = e->state;
        out->pid = e->pid;
        out->last_exit = e->last_exit;
        out->run_count = e->run_count;
        out->fail_count = e->fail_count;
        out->restart_count = e->restart_count;
        out->app_index = e->app_index;
    }
    return HAEUN_SM_OK;
}

/**
 * Stop (if running) then start; respects restart counters.
 */
haeun_sm_result haeun_sm_restart(haeun_sm_hub *hub, int id,
                                 uint32_t caller_pid) {
    haeun_sm_entry *e;
    if (hub == NULL || id < 0 || id >= hub->count || !hub->tab[id].used)
        return HAEUN_SM_NOENT;
    e = &hub->tab[id];
    (void)haeun_sm_status(hub, id, NULL);
    if (e->state == HAEUN_SM_RUNNING || e->state == HAEUN_SM_STARTING) {
        if (haeun_sm_stop(hub, id, caller_pid) != HAEUN_SM_OK)
            return HAEUN_SM_INVALID;
    }
    e->restart_count = 0u;
    return haeun_sm_start(hub, id);
}

/**
 * Called when a pid exits/faults (mux reap).
 */
void haeun_sm_on_pid_exit(haeun_sm_hub *hub, uint32_t pid, uint32_t exit_code,
                          int was_fault) {
    int i;
    if (hub == NULL || pid == 0u)
        return;
    for (i = 0; i < hub->count; ++i) {
        haeun_sm_entry *e = &hub->tab[i];
        if (!e->used || e->pid != pid)
            continue;
        e->last_exit = exit_code;
        e->pid = 0u;
        if (e->state == HAEUN_SM_STOPPING && !was_fault) {
            e->state = HAEUN_SM_STOPPED;
            e->pending_restart = 0u;
        } else {
            e->state = HAEUN_SM_FAILED;
            e->fail_count++;
            e->pending_restart = 0u;
            if (e->restart_policy == HAEUN_SM_RESTART_ALWAYS)
                e->pending_restart = 1u;
            else if (e->restart_policy == HAEUN_SM_RESTART_ON_FAIL &&
                     (was_fault || exit_code != 0u))
                e->pending_restart = 1u;
        }
        return;
    }
}

/** Starts all AUTO services that are STOPPED (boot assist). */
void haeun_sm_autostart(haeun_sm_hub *hub) {
    int i;
    if (hub == NULL)
        return;
    for (i = 0; i < hub->count; ++i) {
        if (hub->tab[i].used &&
            hub->tab[i].startup_type == HAEUN_SM_START_AUTO &&
            hub->tab[i].state == HAEUN_SM_STOPPED)
            (void)haeun_sm_start(hub, i);
    }
}
