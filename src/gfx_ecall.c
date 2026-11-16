#include "haeun/gfx_ecall.h"
#include "haeun/guest_pcb.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <stdio.h>

static void gfx_log(const char *detail) {
    fprintf(stderr, "[haeun-gfx-ecall] failure: %s\n",
            detail != NULL ? detail : "");
}

static uint32_t gfx_to_ret(haeun_gfx_result r, uint32_t ok_value) {
    if (r == HAEUN_GFX_OK)
        return ok_value;
    return 0xffffffffu;
}

static int current_pid(haeun_vm *vm) {
    return (int)haeun_guest_pcb_current_pid(vm);
}

static int deny_foreign(haeun_gfx_wm *wm, haeun_vm *vm, int id) {
    int pid = current_pid(vm);
    /* No PCB context: allow (legacy single-process demos / unit tests). */
    if (pid == 0)
        return 0;
    if (!haeun_gfx_wm_owned_by(wm, id, pid)) {
        gfx_log("window ownership denied");
        return 1;
    }
    return 0;
}

/**
 * Completes GUI window syscalls on the host when a gfx ECALL context is set.
 */
bool haeun_gfx_ecall_io(haeun_vm *vm, void *context) {
    haeun_gfx_ecall_ctx *ctx = (haeun_gfx_ecall_ctx *)context;
    uint32_t number;
    uint32_t a0, a1, a2, a3, a4, a5;
    int id = -1;
    int pid;
    haeun_gfx_result r;
    if (vm == NULL || ctx == NULL || ctx->gfx == NULL || ctx->wm == NULL)
        return false;
    if (vm->privilege != HAEUN_PRIV_USER)
        return false;
    number = haeun_rf_read(vm, 1u);
    a0 = haeun_rf_read(vm, 2u);
    a1 = haeun_rf_read(vm, 3u);
    a2 = haeun_rf_read(vm, 4u);
    a3 = haeun_rf_read(vm, 5u);
    a4 = haeun_rf_read(vm, 6u);
    a5 = haeun_rf_read(vm, 7u);
    switch (number) {
    case HAEUN_SYS_WIN_CREATE:
        if (!haeun_gfx_fb_ready(ctx->gfx)) {
            gfx_log("fb not ready");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        pid = current_pid(vm);
        /*
         * ABI unchanged (guest still passes PA in r6). Process apps (pid!=0)
         * must use the window pool; all callers need RAM/align/FB/overlap OK.
         */
        if (!haeun_gfx_wm_backing_allowed(ctx->wm, ctx->gfx, a4, a2, a3)) {
            gfx_log("win_create backing rejected");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (pid != 0 && !haeun_gfx_wm_in_user_pool(a4, a2, a3)) {
            gfx_log("win_create PA outside user pool");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_gfx_wm_create(ctx->wm, (int32_t)a0, (int32_t)a1, a2, a3, a4,
                                &id);
        if (r == HAEUN_GFX_OK) {
            if (pid != 0)
                (void)haeun_gfx_wm_set_owner(ctx->wm, id, pid);
            if (pid != 0) {
                if (ctx->evt != NULL)
                    haeun_evt_win_post(ctx->evt, id, HAEUN_EVT_CREATED,
                                       (uint32_t)a0, (uint32_t)a1);
                (void)haeun_gfx_wm_raise(ctx->wm, id);
                (void)haeun_gfx_wm_set_focus(ctx->wm, id);
                if (ctx->evt != NULL)
                    haeun_evt_on_focus(ctx->evt, id);
            }
        }
        haeun_rf_write(vm, 1u, gfx_to_ret(r, (uint32_t)id));
        return true;
    case HAEUN_SYS_WIN_DESTROY:
        if (deny_foreign(ctx->wm, vm, (int)a0)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        if (ctx->evt != NULL && ctx->wm->windows[(int)a0].used)
            haeun_evt_win_post(ctx->evt, (int)a0, HAEUN_EVT_DESTROYED, 0u, 0u);
        r = haeun_gfx_wm_destroy(ctx->wm, (int)a0);
        if (r == HAEUN_GFX_OK && ctx->evt != NULL)
            haeun_evt_sync_focus(ctx->evt);
        haeun_rf_write(vm, 1u, gfx_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_WIN_FILL:
        if (deny_foreign(ctx->wm, vm, (int)a0)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_gfx_wm_fill(ctx->wm, (int)a0, a1);
        haeun_rf_write(vm, 1u, gfx_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_WIN_FILL_RECT:
        if (deny_foreign(ctx->wm, vm, (int)a0)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_gfx_wm_fill_rect(ctx->wm, (int)a0, a1, a2, a3, a4, a5);
        haeun_rf_write(vm, 1u, gfx_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_WIN_PIXEL:
        if (deny_foreign(ctx->wm, vm, (int)a0)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_gfx_wm_put_pixel(ctx->wm, (int)a0, a1, a2, a3);
        haeun_rf_write(vm, 1u, gfx_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_WIN_RAISE:
        if (deny_foreign(ctx->wm, vm, (int)a0)) {
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_gfx_wm_raise(ctx->wm, (int)a0);
        /* M21: owner raise also claims focus so desktop regains input after apps. */
        if (r == HAEUN_GFX_OK) {
            (void)haeun_gfx_wm_set_focus(ctx->wm, (int)a0);
            if (ctx->evt != NULL)
                haeun_evt_on_focus(ctx->evt, (int)a0);
        }
        haeun_rf_write(vm, 1u, gfx_to_ret(r, 0u));
        return true;
    case HAEUN_SYS_GFX_COMPOSE:
        if (!haeun_gfx_fb_ready(ctx->gfx)) {
            gfx_log("compose fb not ready");
            haeun_rf_write(vm, 1u, 0xffffffffu);
            return true;
        }
        r = haeun_gfx_compose(ctx->gfx, ctx->wm, a0);
        haeun_rf_write(vm, 1u, gfx_to_ret(r, 0u));
        return true;
    default:
        return false;
    }
}
