#ifndef HAEUN_GFX_ECALL_H
#define HAEUN_GFX_ECALL_H

#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_wm.h"
#include "haeun/vm.h"

#include <stdbool.h>

typedef struct {
    haeun_gfx *gfx;
    haeun_gfx_wm *wm;
    haeun_evt_hub *evt; /* optional; auto-focus + sync on create/destroy */
} haeun_gfx_ecall_ctx;

/**
 * Host ECALL assist for WIN_CREATE/DESTROY/FILL/PIXEL/RAISE and GFX_COMPOSE.
 * WIN_CREATE: r2=x r3=y r4=w r5=h r6=backing PA → window id.
 * Context must hold an enabled framebuffer gfx and initialized wm.
 */
bool haeun_gfx_ecall_io(haeun_vm *vm, void *context);

#endif
