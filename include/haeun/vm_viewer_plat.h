#ifndef HAEUN_VM_VIEWER_PLAT_H
#define HAEUN_VM_VIEWER_PLAT_H

#include "haeun/vm_viewer.h"

#include <stddef.h>
#include <stdint.h>
#include <windows.h>

typedef struct {
    HWND hwnd;
    haeun_vm_viewer *viewer;
    uint8_t *bgra;
    size_t bgra_cap;
    uint32_t fb_w;
    uint32_t fb_h;
    int quit;
    HDC mem_dc;
    HBITMAP mem_bmp;
    HBITMAP mem_old;
    int mem_w;
    int mem_h;
} viewer_plat;

/** Blit the cached BGRA FB into the Win32 client without flicker. */
void haeun_viewer_paint(viewer_plat *p);

/** Compose (if chrome), snapshot FB, then paint. */
void haeun_viewer_refresh_fb(viewer_plat *p);

/** Releases cached GDI backbuffer objects. */
void haeun_viewer_blit_fini(viewer_plat *p);

#endif
