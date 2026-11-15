#include "haeun/vm_gui_host.h"

#include <string.h>

/**
 * Enables FB + WM + optional M12 chrome; installs gfx ECALL mux.
 */
haeun_gfx_result haeun_vm_gui_host_attach_ex(haeun_vm_gui_host *host,
                                             haeun_vm *vm, int desktop_mode) {
    haeun_gfx_result r;
    if (host == NULL || vm == NULL)
        return HAEUN_GFX_BAD_ARGUMENT;
    memset(host, 0, sizeof(*host));
    r = haeun_gfx_fb_enable(&host->gfx, vm, HAEUN_GUI_FB_WIDTH,
                            HAEUN_GUI_FB_HEIGHT, HAEUN_GUI_FB_BASE);
    if (r != HAEUN_GFX_OK)
        return r;
    r = haeun_gfx_wm_init(&host->wm, vm);
    if (r != HAEUN_GFX_OK)
        return r;
    if (desktop_mode) {
        r = haeun_gfx_chrome_init(&host->chrome, &host->gfx, &host->wm);
        if (r != HAEUN_GFX_OK)
            return r;
        host->desktop = 1;
    }
    host->gfx_ctx.gfx = &host->gfx;
    host->gfx_ctx.wm = &host->wm;
    haeun_evt_hub_init(&host->evt, &host->wm);
    host->gfx_ctx.evt = &host->evt;
    haeun_ipc_hub_init(&host->ipc);
    haeun_svc_hub_init(&host->svc);
    host->m20_ctx.evt = &host->evt;
    host->m20_ctx.ipc = &host->ipc;
    host->m20_ctx.svc = &host->svc;
    host->m20_ctx.gfx = &host->gfx;
    host->m20_ctx.wm = &host->wm;
    if (desktop_mode)
        host->chrome.evt = &host->evt;
    host->mux.proc_ft = NULL;
    host->mux.gfx_ctx = &host->gfx_ctx;
    host->mux.sock_ctx = NULL;
    host->mux.app_env = NULL;
    host->mux.m20_ctx = &host->m20_ctx;
    haeun_vm_set_ecall_io(vm, haeun_ecall_mux_io, &host->mux);
    haeun_vm_set_fault_io(vm, haeun_ecall_mux_on_fault, &host->mux);
    host->attached = 1;
    return HAEUN_GFX_OK;
}

/**
 * Default attach without desktop chrome (M7/M8 compatible).
 */
haeun_gfx_result haeun_vm_gui_host_attach(haeun_vm_gui_host *host,
                                          haeun_vm *vm) {
    return haeun_vm_gui_host_attach_ex(host, vm, 0);
}

/**
 * Desktop + application environment (registry, HFS, APP_* ECALLs).
 */
haeun_gfx_result haeun_vm_gui_host_attach_apps(haeun_vm_gui_host *host,
                                               haeun_vm *vm) {
    haeun_gfx_result r;
    if (haeun_vm_gui_host_attach_ex(host, vm, 1) != HAEUN_GFX_OK)
        return HAEUN_GFX_BAD_ARGUMENT;
    if (haeun_hfs_mkfs(host->hfs_image, sizeof(host->hfs_image), 32u) !=
        HAEUN_HFS_OK)
        return HAEUN_GFX_BAD_ARGUMENT;
    if (haeun_hfs_mount(&host->hfs, host->hfs_image, sizeof(host->hfs_image)) !=
        HAEUN_HFS_OK)
        return HAEUN_GFX_BAD_ARGUMENT;
    haeun_proc_ft_init(&host->proc_ft, &host->hfs);
    if (haeun_app_env_init(&host->apps, vm, &host->wm) != HAEUN_APP_OK)
        return HAEUN_GFX_BAD_ARGUMENT;
    host->apps.evt = &host->evt;
    if (haeun_sm_init(&host->sm, vm, &host->apps, &host->evt) != HAEUN_SM_OK)
        return HAEUN_GFX_BAD_ARGUMENT;
    host->mux.proc_ft = &host->proc_ft;
    host->mux.app_env = &host->apps;
    host->mux.sm = &host->sm;
    host->apps_mode = 1;
    (void)r;
    return HAEUN_GFX_OK;
}

/**
 * Registers a named application HXF with the apps-mode host.
 */
haeun_app_result haeun_vm_gui_host_register_app(haeun_vm_gui_host *host,
                                                const char *name,
                                                const uint8_t *image,
                                                size_t image_size) {
    if (host == NULL || !host->apps_mode)
        return HAEUN_APP_BAD_ARGUMENT;
    return haeun_app_register(&host->apps, name, image, image_size);
}
