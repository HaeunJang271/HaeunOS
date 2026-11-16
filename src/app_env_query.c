#include "haeun/app_env.h"

#include <string.h>

enum {
    PCB_BASE = 0x0000A000u,
    PCB_STRIDE = 0x100u,
    PCB_NPROC = 4
};

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void copy_field(char *dst, size_t cap, const char *src) {
    if (dst == NULL || cap == 0u)
        return;
    if (src == NULL)
        src = "";
    strncpy(dst, src, cap - 1u);
    dst[cap - 1u] = '\0';
}

/**
 * Sets optional metadata for a registered index (display name unchanged).
 */
haeun_app_result haeun_app_set_meta(haeun_app_env *env, int index,
                                    const char *desc, const char *version,
                                    const char *category, uint32_t flags) {
    haeun_app_entry *e;
    if (env == NULL || index < 0 || index >= env->app_count ||
        !env->apps[index].used)
        return HAEUN_APP_NOENT;
    e = &env->apps[index];
    if (desc != NULL)
        copy_field(e->desc, sizeof(e->desc), desc);
    if (version != NULL)
        copy_field(e->version, sizeof(e->version), version);
    if (category != NULL)
        copy_field(e->category, sizeof(e->category), category);
    e->flags = flags;
    return HAEUN_APP_OK;
}

/**
 * Copies an APP_INFO string field, or writes *flags_out for FIELD_FLAGS.
 */
haeun_app_result haeun_app_info(const haeun_app_env *env, int index, int field,
                                char *out, size_t out_cap, uint32_t *flags_out) {
    const haeun_app_entry *e;
    if (env == NULL || index < 0 || index >= env->app_count ||
        !env->apps[index].used)
        return HAEUN_APP_NOENT;
    e = &env->apps[index];
    if (field == HAEUN_APP_FIELD_FLAGS) {
        if (flags_out == NULL)
            return HAEUN_APP_BAD_ARGUMENT;
        *flags_out = e->flags;
        return HAEUN_APP_OK;
    }
    if (out == NULL || out_cap == 0u)
        return HAEUN_APP_BAD_ARGUMENT;
    switch (field) {
    case HAEUN_APP_FIELD_NAME:
        copy_field(out, out_cap, e->name);
        break;
    case HAEUN_APP_FIELD_DESC:
        copy_field(out, out_cap, e->desc);
        break;
    case HAEUN_APP_FIELD_VER:
        copy_field(out, out_cap, e->version);
        break;
    case HAEUN_APP_FIELD_CAT:
        copy_field(out, out_cap, e->category);
        break;
    default:
        return HAEUN_APP_BAD_ARGUMENT;
    }
    return HAEUN_APP_OK;
}

static int find_win(const haeun_gfx_wm *wm, uint32_t pid) {
    int i;
    if (wm == NULL || pid == 0u)
        return -1;
    for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
        if (wm->windows[i].used && wm->windows[i].owner_pid == (int)pid)
            return i;
    }
    return -1;
}

static int pcb_by_pid(const haeun_vm *vm, uint32_t pid, uint32_t *state,
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

static int caller_is_parent(const haeun_vm *vm, uint32_t child_pid,
                            uint32_t caller_pid) {
    int i;
    if (vm == NULL || vm->ram == NULL || child_pid == 0u || caller_pid == 0u)
        return 0;
    for (i = 0; i < PCB_NPROC; ++i) {
        uint32_t off = PCB_BASE + (uint32_t)i * PCB_STRIDE;
        if ((uint64_t)off + 12u > vm->ram_size)
            return 0;
        if (rd32(vm->ram + off + 4u) == child_pid)
            return rd32(vm->ram + off + 8u) == caller_pid ? 1 : 0;
    }
    return 0;
}

/**
 * Fills run info for AS slot (0..SLOTS-1). Empty/free slots return NOENT.
 */
haeun_app_result haeun_app_run_at(const haeun_app_env *env, int slot,
                                  haeun_app_run_info *out) {
    uint32_t state = 0u;
    uint32_t code = 0u;
    if (env == NULL || out == NULL || slot < 0 || slot >= HAEUN_APP_SLOTS)
        return HAEUN_APP_BAD_ARGUMENT;
    if (!env->runs[slot].used || env->runs[slot].pid == 0u)
        return HAEUN_APP_NOENT;
    memset(out, 0, sizeof(*out));
    out->pid = env->runs[slot].pid;
    out->app_index = env->runs[slot].app_index;
    out->win_id = find_win(env->wm, out->pid);
    if (pcb_by_pid(env->vm, out->pid, &state, &code) >= 0) {
        out->state = state;
        out->exit_code = code;
    } else {
        out->state = 0u;
        out->exit_code = 0u;
    }
    return HAEUN_APP_OK;
}

/**
 * Parent-only focus (op FOCUS) or soft CLOSE_REQUEST (op CLOSE) for child pid.
 */
haeun_app_result haeun_app_ctrl(haeun_app_env *env, int op, uint32_t target_pid,
                                uint32_t caller_pid) {
    int wid;
    int i;
    int found = 0;
    if (env == NULL || target_pid == 0u || caller_pid == 0u)
        return HAEUN_APP_BAD_ARGUMENT;
    if (!caller_is_parent(env->vm, target_pid, caller_pid))
        return HAEUN_APP_INVALID;
    for (i = 0; i < HAEUN_APP_SLOTS; ++i) {
        if (env->runs[i].used && env->runs[i].pid == target_pid) {
            found = 1;
            break;
        }
    }
    if (!found)
        return HAEUN_APP_NOENT;
    if (op == HAEUN_APP_CTRL_FOCUS) {
        wid = find_win(env->wm, target_pid);
        if (wid < 0 || env->wm == NULL)
            return HAEUN_APP_NOENT;
        (void)haeun_gfx_wm_raise(env->wm, wid);
        (void)haeun_gfx_wm_set_focus(env->wm, wid);
        if (env->evt != NULL)
            haeun_evt_on_focus(env->evt, wid);
        return HAEUN_APP_OK;
    }
    if (op == HAEUN_APP_CTRL_CLOSE) {
        haeun_evt ev;
        int closed_win = 0;
        /* Queue CLOSE first so the app exits via EVENT_POLL even with no
         * window yet; then tear down any windows it already owns. */
        if (env->evt != NULL) {
            memset(&ev, 0, sizeof(ev));
            ev.type = (uint16_t)HAEUN_EVT_CLOSE;
            ev.win = (int16_t)-1;
            (void)haeun_evt_push(env->evt, (int)target_pid, &ev);
        }
        if (env->wm != NULL) {
            for (i = 0; i < HAEUN_GFX_WM_MAX_WINDOWS; ++i) {
                if (env->wm->windows[i].used &&
                    env->wm->windows[i].owner_pid == (int)target_pid) {
                    (void)haeun_gfx_wm_destroy(env->wm, i);
                    closed_win = 1;
                }
            }
            if (closed_win && env->evt != NULL)
                haeun_evt_sync_focus(env->evt);
        }
        return HAEUN_APP_OK;
    }
    return HAEUN_APP_BAD_ARGUMENT;
}
