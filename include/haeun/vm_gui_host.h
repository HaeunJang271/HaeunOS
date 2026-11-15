#ifndef HAEUN_VM_GUI_HOST_H
#define HAEUN_VM_GUI_HOST_H

#include "haeun/app_env.h"
#include "haeun/ecall_mux.h"
#include "haeun/evt.h"
#include "haeun/gfx.h"
#include "haeun/gfx_chrome.h"
#include "haeun/gfx_ecall.h"
#include "haeun/gfx_wm.h"
#include "haeun/hfs.h"
#include "haeun/ipc.h"
#include "haeun/m20_ecall.h"
#include "haeun/proc_hfs.h"
#include "haeun/svc.h"
#include "haeun/svc_mgr.h"
#include "haeun/vm.h"

/** Default framebuffer geometry for --gui / gui_demo (must match guest). */
enum {
    HAEUN_GUI_FB_WIDTH = 64u,
    HAEUN_GUI_FB_HEIGHT = 32u,
    HAEUN_GUI_FB_BASE = 0x10000u,
    HAEUN_APPS_HFS_BYTES = 32u * 512u
};

typedef struct {
    haeun_gfx gfx;
    haeun_gfx_wm wm;
    haeun_gfx_chrome chrome;
    haeun_gfx_ecall_ctx gfx_ctx;
    haeun_ecall_mux_ctx mux;
    haeun_app_env apps;
    haeun_hfs hfs;
    haeun_proc_ft proc_ft;
    haeun_evt_hub evt;
    haeun_ipc_hub ipc;
    haeun_svc_hub svc;
    haeun_sm_hub sm;
    haeun_m20_ecall_ctx m20_ctx;
    uint8_t hfs_image[HAEUN_APPS_HFS_BYTES];
    int desktop;
    int apps_mode;
    int attached;
} haeun_vm_gui_host;

/**
 * Enables the framebuffer, initializes the WM, and installs the ECALL mux
 * with a graphics assist (process/socket slots left NULL).
 */
haeun_gfx_result haeun_vm_gui_host_attach(haeun_vm_gui_host *host,
                                          haeun_vm *vm);

/**
 * Like attach, and enables desktop chrome when desktop_mode is nonzero.
 */
haeun_gfx_result haeun_vm_gui_host_attach_ex(haeun_vm_gui_host *host,
                                             haeun_vm *vm, int desktop_mode);

/**
 * Desktop + application environment (registry, HFS, APP_* ECALLs).
 */
haeun_gfx_result haeun_vm_gui_host_attach_apps(haeun_vm_gui_host *host,
                                               haeun_vm *vm);

/**
 * Registers a named application HXF with the apps-mode host.
 */
haeun_app_result haeun_vm_gui_host_register_app(haeun_vm_gui_host *host,
                                                const char *name,
                                                const uint8_t *image,
                                                size_t image_size);

#endif
