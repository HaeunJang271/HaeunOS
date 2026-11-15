#ifndef HAEUN_FB_VIEW_H
#define HAEUN_FB_VIEW_H

#include "haeun/vm.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAEUN_FB_VIEW_OK = 0,
    HAEUN_FB_VIEW_BAD_ARGUMENT,
    HAEUN_FB_VIEW_NOT_READY,
    HAEUN_FB_VIEW_OOB,
    HAEUN_FB_VIEW_NOSPACE
} haeun_fb_view_result;

/**
 * Copies guest RGBA8888 (LE) framebuffer into a tightly packed BGRA8888
 * top-down buffer suitable for Win32 BI_RGB DIBs. out_cap must be >= w*h*4.
 */
haeun_fb_view_result haeun_fb_view_snapshot_bgra(const haeun_vm *vm,
                                                 uint8_t *out_bgra,
                                                 size_t out_cap, uint32_t *out_w,
                                                 uint32_t *out_h);

/**
 * Maps window client pixels into framebuffer coordinates using integer scale.
 * Returns 0 on success; -1 if outside the scaled FB rect.
 */
int haeun_fb_view_map_mouse(int client_x, int client_y, int win_w, int win_h,
                            uint32_t fb_w, uint32_t fb_h, int32_t *fb_x,
                            int32_t *fb_y);

#endif
