#ifndef HAEUN_M20_ECALL_H
#define HAEUN_M20_ECALL_H

#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/ipc.h"
#include "haeun/svc.h"
#include "haeun/vm.h"

#include <stdbool.h>

typedef struct {
    haeun_evt_hub *evt;
    haeun_ipc_hub *ipc;
    haeun_svc_hub *svc;
    haeun_gfx *gfx;
    haeun_gfx_wm *wm;
} haeun_m20_ecall_ctx;

/**
 * Host assist for EVENT_POLL, WIN_DRAW_TEXT, IPC_*, SVC_*, WIN_QUERY (27–37).
 */
bool haeun_m20_ecall_io(haeun_vm *vm, void *context);

#endif
