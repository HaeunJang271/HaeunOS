#ifndef HAEUN_VM_VIEWER_H
#define HAEUN_VM_VIEWER_H

#include "haeun/gfx_chrome.h"
#include "haeun/gfx_wm.h"
#include "haeun/vm.h"
#include "haeun/vm_input_host.h"

#include <stdint.h>

typedef enum {
    HAEUN_VIEWER_OK = 0,
    HAEUN_VIEWER_BAD_ARGUMENT,
    HAEUN_VIEWER_UNSUPPORTED,
    HAEUN_VIEWER_CREATE_FAILED
} haeun_viewer_result;

typedef struct haeun_vm_viewer haeun_vm_viewer;

struct haeun_vm_viewer {
    haeun_vm *vm;
    haeun_gfx_wm *wm;
    haeun_gfx_chrome *chrome;    /* optional M12 desktop chrome */
    haeun_vm_input_host *script; /* optional M10 scripted events */
    int live_input;              /* host WM_* → feed/post */
    int scale;
    int attached;
    int window_open;
    int frames_drawn;
    int keys_fed;
    int mice_posted;
    int focus_hits;
    uint32_t mouse_buttons;
    void *platform; /* HWND etc.; opaque */
};

/**
 * Prepares a cooperative Win32 viewer (poll-only input; IRQ left disabled).
 * Returns HAEUN_VIEWER_UNSUPPORTED on non-Windows builds.
 */
haeun_viewer_result haeun_vm_viewer_attach(haeun_vm_viewer *viewer, haeun_vm *vm,
                                           haeun_gfx_wm *wm, int live_input);

/**
 * Opens the window, steps the VM in slices, pumps messages, and blits the FB.
 * Optional script injects M10 timeline events. Closes the window on return.
 */
haeun_vm_result haeun_vm_viewer_run(haeun_vm_viewer *viewer, haeun_vm *vm,
                                    uint64_t max_steps, uint64_t *steps_executed);

#endif
