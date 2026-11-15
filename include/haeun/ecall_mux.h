#ifndef HAEUN_ECALL_MUX_H
#define HAEUN_ECALL_MUX_H

#include "haeun/app_env.h"
#include "haeun/gfx_ecall.h"
#include "haeun/m20_ecall.h"
#include "haeun/net_sock_ecall.h"
#include "haeun/proc_hfs.h"
#include "haeun/svc_mgr.h"
#include "haeun/vm.h"

#include <stdbool.h>

/**
 * Optional host assists for the single VM ecall_io hook.
 * NULL slots are skipped; unknown numbers return false so the trap path runs.
 */
typedef struct {
    haeun_proc_ft *proc_ft;
    haeun_gfx_ecall_ctx *gfx_ctx;
    haeun_sock_ecall_ctx *sock_ctx;
    haeun_app_env *app_env;
    haeun_m20_ecall_ctx *m20_ctx;
    haeun_sm_hub *sm;
} haeun_ecall_mux_ctx;

/**
 * Dispatches one user ECALL to at most one assist by syscall number range.
 * Returns false when no assist claims the call (trap proceeds).
 */
bool haeun_ecall_mux_io(haeun_vm *vm, void *context);

/**
 * Host cleanup+log for a fatal user trap; reuses EXIT reap paths.
 */
void haeun_ecall_mux_on_fault(haeun_vm *vm, void *context, uint32_t cause,
                              uint32_t tval, uint32_t epc);

#endif
