#include "haeun/ecall_mux.h"
#include "haeun/guest_pcb.h"
#include "haeun/register_file.h"
#include "haeun/sec.h"

#include <stdio.h>

static void reap_pid(haeun_ecall_mux_ctx *ctx, uint32_t pid, uint32_t exit_code,
                     int was_fault) {
    if (ctx == NULL || pid == 0u)
        return;
    if (ctx->sm != NULL)
        haeun_sm_on_pid_exit(ctx->sm, pid, exit_code, was_fault);
    if (ctx->sock_ctx != NULL && ctx->sock_ctx->ss != NULL)
        (void)haeun_socks_close_by_owner(ctx->sock_ctx->ss, (int)pid);
    if (ctx->proc_ft != NULL)
        (void)haeun_proc_fds_close_by_owner(ctx->proc_ft, (int)pid);
    if (ctx->m20_ctx != NULL) {
        if (ctx->m20_ctx->evt != NULL)
            haeun_evt_reap(ctx->m20_ctx->evt, (int)pid);
        if (ctx->m20_ctx->ipc != NULL)
            haeun_ipc_reap(ctx->m20_ctx->ipc, (int)pid);
        if (ctx->m20_ctx->svc != NULL)
            haeun_svc_reap(ctx->m20_ctx->svc, (int)pid);
    }
    if (ctx->app_env != NULL)
        (void)haeun_app_reap(ctx->app_env, pid);
    else if (ctx->gfx_ctx != NULL && ctx->gfx_ctx->wm != NULL)
        (void)haeun_app_destroy_windows(ctx->gfx_ctx->wm, pid);
    if (ctx->m20_ctx != NULL && ctx->m20_ctx->evt != NULL)
        haeun_evt_sync_focus(ctx->m20_ctx->evt);
}

/**
 * Logs and reaps host-owned resources for the current PCB after a user fault.
 * When tval falls in the page below PCB.ustack_base, tags stack-overflow.
 */
void haeun_ecall_mux_on_fault(haeun_vm *vm, void *context, uint32_t cause,
                              uint32_t tval, uint32_t epc) {
    haeun_ecall_mux_ctx *ctx = (haeun_ecall_mux_ctx *)context;
    uint32_t pid;
    uint32_t curr;
    uint32_t base;
    const char *kind = "user";
    if (vm == NULL || ctx == NULL)
        return;
    pid = haeun_guest_pcb_current_pid(vm);
    if (vm->ram != NULL && vm->ram_size >= 0xA404u) {
        curr = (uint32_t)vm->ram[0xA400] | ((uint32_t)vm->ram[0xA401] << 8) |
               ((uint32_t)vm->ram[0xA402] << 16) |
               ((uint32_t)vm->ram[0xA403] << 24);
        if (curr < 4u) {
            uint32_t off = 0xA000u + curr * 0x100u + 40u;
            if ((uint64_t)off + 4u <= (uint64_t)vm->ram_size) {
                base = (uint32_t)vm->ram[off] |
                       ((uint32_t)vm->ram[off + 1u] << 8) |
                       ((uint32_t)vm->ram[off + 2u] << 16) |
                       ((uint32_t)vm->ram[off + 3u] << 24);
                if (base >= 4096u && tval >= (base - 4096u) && tval < base)
                    kind = "stack-overflow";
            }
        }
    }
    (void)fprintf(stderr,
                  "[haeun-fault] %s pid=%u cause=%u epc=0x%08x tval=0x%08x\n",
                  kind, pid, cause, epc, tval);
    reap_pid(ctx, pid, cause, 1);
}

/**
 * Routes a single ECALL to exactly one installed assist, or declines it.
 */
bool haeun_ecall_mux_io(haeun_vm *vm, void *context) {
    haeun_ecall_mux_ctx *ctx = (haeun_ecall_mux_ctx *)context;
    uint32_t number;
    uint32_t pid;
    if (vm == NULL || ctx == NULL)
        return false;
    number = haeun_rf_read(vm, 1u);
    if (number == (uint32_t)HAEUN_SYS_EXIT) {
        pid = haeun_guest_pcb_current_pid(vm);
        reap_pid(ctx, pid, haeun_rf_read(vm, 2u), 0);
        return false;
    }
    if (number >= (uint32_t)HAEUN_SYS_SOCKET &&
        number <= (uint32_t)HAEUN_SYS_SO_CLOSE) {
        if (ctx->sock_ctx == NULL)
            return false;
        return haeun_sock_ecall_io(vm, ctx->sock_ctx);
    }
    if ((number >= (uint32_t)HAEUN_SYS_WIN_CREATE &&
         number <= (uint32_t)HAEUN_SYS_GFX_COMPOSE) ||
        number == (uint32_t)HAEUN_SYS_WIN_FILL_RECT) {
        if (ctx->gfx_ctx == NULL)
            return false;
        return haeun_gfx_ecall_io(vm, ctx->gfx_ctx);
    }
    if ((number >= (uint32_t)HAEUN_SYS_APP_COUNT &&
         number <= (uint32_t)HAEUN_SYS_APP_SPAWN) ||
        (number >= (uint32_t)HAEUN_SYS_APP_INFO &&
         number <= (uint32_t)HAEUN_SYS_APP_CTRL)) {
        if (ctx->app_env == NULL)
            return false;
        return haeun_app_ecall_io(vm, ctx->app_env);
    }
    if (number >= (uint32_t)HAEUN_SYS_EVENT_POLL &&
        number <= (uint32_t)HAEUN_SYS_WIN_QUERY) {
        if (ctx->m20_ctx == NULL)
            return false;
        return haeun_m20_ecall_io(vm, ctx->m20_ctx);
    }
    if (number >= (uint32_t)HAEUN_SYS_SERVICE_COUNT &&
        number <= (uint32_t)HAEUN_SYS_SERVICE_RESTART) {
        if (ctx->sm == NULL)
            return false;
        return haeun_sm_ecall_io(vm, ctx->sm);
    }
    if (ctx->proc_ft == NULL)
        return false;
    return haeun_proc_ecall_io(vm, ctx->proc_ft);
}
