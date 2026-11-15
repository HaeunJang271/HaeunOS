#include "haeun/vm_viewer.h"

#include <string.h>

/**
 * Non-Windows stub: viewer is unavailable without a host window backend.
 */
haeun_viewer_result haeun_vm_viewer_attach(haeun_vm_viewer *viewer, haeun_vm *vm,
                                           haeun_gfx_wm *wm, int live_input) {
    (void)vm;
    (void)wm;
    (void)live_input;
    if (viewer != NULL)
        memset(viewer, 0, sizeof(*viewer));
    return HAEUN_VIEWER_UNSUPPORTED;
}

/**
 * Stub run path; never starts the guest under a window.
 */
haeun_vm_result haeun_vm_viewer_run(haeun_vm_viewer *viewer, haeun_vm *vm,
                                    uint64_t max_steps,
                                    uint64_t *steps_executed) {
    (void)viewer;
    (void)vm;
    (void)max_steps;
    if (steps_executed != NULL)
        *steps_executed = 0u;
    return HAEUN_VM_BAD_ARGUMENT;
}
