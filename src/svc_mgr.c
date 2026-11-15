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

static void copy_str(char *dst, size_t cap, const char *src) {
    if (dst == NULL || cap == 0u)
        return;
    if (src == NULL)
        src = "";
    strncpy(dst, src, cap - 1u);
    dst[cap - 1u] = '\0';
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

static haeun_sm_result stop_req(haeun_sm_hub *hub, int id, uint32_t caller_pid,
                                int check_parent);

/**
 * Binds the service manager to the app environment (spawn/reap path).
 */
haeun_sm_result haeun_sm_init(haeun_sm_hub *hub, haeun_vm *vm,
                              haeun_app_env *apps, haeun_evt_hub *evt) {
    if (hub == NULL || vm == NULL || apps == NULL)
        return HAEUN_SM_BAD_ARGUMENT;
    memset(hub, 0, sizeof(*hub));
    hub->vm = vm;
    hub->apps = apps;
    hub->evt = evt;
    return HAEUN_SM_OK;
}

/**
 * Registers a service bound to an existing app registry index.
 */
haeun_sm_result haeun_sm_register(haeun_sm_hub *hub, const char *name,
                                  const char *desc, const char *version,
                                  int app_index, uint32_t startup_type,
                                  uint32_t restart_policy) {
    haeun_sm_entry *e;
    int i;
    if (hub == NULL || name == NULL || name[0] == '\0' || hub->apps == NULL)
        return HAEUN_SM_BAD_ARGUMENT;
    if (app_index < 0 || app_index >= haeun_app_count(hub->apps))
        return HAEUN_SM_INVALID;
    if (hub->count >= HAEUN_SM_MAX)
        return HAEUN_SM_NOSPC;
    for (i = 0; i < hub->count; ++i) {
        if (hub->tab[i].used && strcmp(hub->tab[i].name, name) == 0)
            return HAEUN_SM_INVALID;
    }
    e = &hub->tab[hub->count];
    memset(e, 0, sizeof(*e));
    e->used = 1;
    copy_str(e->name, sizeof(e->name), name);
    copy_str(e->desc, sizeof(e->desc), desc != NULL ? desc : "");
    copy_str(e->version, sizeof(e->version), version != NULL ? version : "1");
    e->app_index = app_index;
    e->startup_type = startup_type;
    e->restart_policy = restart_policy;
    e->state = HAEUN_SM_STOPPED;
    e->max_restarts = HAEUN_SM_MAX_RESTARTS;
    hub->count++;
    return HAEUN_SM_OK;
}

/** Returns registered service count. */
int haeun_sm_count(const haeun_sm_hub *hub) {
    return hub != NULL ? hub->count : 0;
}

/** Copies a string INFO field, or packs flags into *flags_out. */
haeun_sm_result haeun_sm_info(const haeun_sm_hub *hub, int id, int field,
                              char *out, uint32_t out_cap, uint32_t *flags_out) {
    const haeun_sm_entry *e;
    if (hub == NULL || id < 0 || id >= hub->count || !hub->tab[id].used)
        return HAEUN_SM_NOENT;
    e = &hub->tab[id];
    if (field == HAEUN_SM_FIELD_FLAGS) {
        if (flags_out == NULL)
            return HAEUN_SM_BAD_ARGUMENT;
        *flags_out = (e->startup_type & 0xffu) |
                     ((e->restart_policy & 0xffu) << 8) |
                     ((e->app_index & 0xffu) << 16);
        return HAEUN_SM_OK;
    }
    if (out == NULL || out_cap == 0u)
        return HAEUN_SM_BAD_ARGUMENT;
    switch (field) {
    case HAEUN_SM_FIELD_NAME:
        copy_str(out, out_cap, e->name);
        break;
    case HAEUN_SM_FIELD_DESC:
        copy_str(out, out_cap, e->desc);
        break;
    case HAEUN_SM_FIELD_VER:
        copy_str(out, out_cap, e->version);
        break;
    default:
        return HAEUN_SM_BAD_ARGUMENT;
    }
    return HAEUN_SM_OK;
}

static int live_pcb(uint32_t st) {
    return st == STATE_READY || st == STATE_RUNNING || st == 3u || st == 4u;
}

/**
 * Spawns the service's app; links PID.
 */
haeun_sm_result haeun_sm_start(haeun_sm_hub *hub, int id) {
    haeun_sm_entry *e;
    uint32_t pid = 0u;
    uint32_t st = 0u;
    haeun_app_result ar;
    if (hub == NULL || hub->apps == NULL || id < 0 || id >= hub->count ||
        !hub->tab[id].used)
        return HAEUN_SM_NOENT;
    e = &hub->tab[id];
    if (e->state == HAEUN_SM_RUNNING || e->state == HAEUN_SM_STARTING ||
        e->state == HAEUN_SM_STOPPING)
        return HAEUN_SM_BUSY;
    e->state = HAEUN_SM_STARTING;
    ar = haeun_app_spawn(hub->apps, e->app_index, &pid);
    if (ar == HAEUN_APP_NOSPC || ar == HAEUN_APP_BUSY) {
        e->state = HAEUN_SM_STOPPED;
        return HAEUN_SM_NOSPC;
    }
    if (ar != HAEUN_APP_OK) {
        e->state = HAEUN_SM_FAILED;
        e->fail_count++;
        sm_log("start: spawn failed");
        return HAEUN_SM_INVALID;
    }
    if (pcb_lookup(hub->vm, pid, &st, NULL) < 0 || !live_pcb(st)) {
        e->state = HAEUN_SM_FAILED;
        e->fail_count++;
        e->pid = 0u;
        return HAEUN_SM_FAILED_RC;
    }
    e->pid = pid;
    e->generation++;
    e->run_count++;
    e->state = HAEUN_SM_RUNNING;
    return HAEUN_SM_OK;
}

static haeun_sm_result stop_req(haeun_sm_hub *hub, int id, uint32_t caller_pid,
                                int check_parent) {
    haeun_sm_entry *e;
    haeun_evt ev;
    if (hub == NULL || id < 0 || id >= hub->count || !hub->tab[id].used)
        return HAEUN_SM_NOENT;
    e = &hub->tab[id];
    (void)haeun_sm_status(hub, id, NULL);
    if (e->state != HAEUN_SM_RUNNING && e->state != HAEUN_SM_STARTING)
        return HAEUN_SM_INVALID;
    if (e->pid == 0u)
        return HAEUN_SM_INVALID;
    if (check_parent && hub->apps != NULL && caller_pid != 0u) {
        if (haeun_app_ctrl(hub->apps, HAEUN_APP_CTRL_CLOSE, e->pid,
                           caller_pid) == HAEUN_APP_INVALID)
            return HAEUN_SM_INVALID;
    }
    e->state = HAEUN_SM_STOPPING;
    memset(&ev, 0, sizeof(ev));
    ev.type = (uint16_t)HAEUN_EVT_CLOSE;
    ev.win = -1;
    if (hub->evt != NULL)
        (void)haeun_evt_push(hub->evt, (int)e->pid, &ev);
    (void)check_parent;
    (void)caller_pid;
    return HAEUN_SM_OK;
}

/**
 * Requests cooperative stop; does not poke PCB state.
 */
haeun_sm_result haeun_sm_stop(haeun_sm_hub *hub, int id, uint32_t caller_pid) {
    return stop_req(hub, id, caller_pid, 1);
}
