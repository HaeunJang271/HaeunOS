#include "haeun/fb_view.h"

#include <string.h>

static haeun_fb_view_result span(const haeun_vm *vm, uint32_t *w, uint32_t *h,
                                 uint32_t *base, uint64_t *bytes) {
    uint64_t end;
    if (vm == NULL || vm->ram == NULL || w == NULL || h == NULL || base == NULL ||
        bytes == NULL)
        return HAEUN_FB_VIEW_BAD_ARGUMENT;
    if ((vm->fb_control & 1u) == 0u)
        return HAEUN_FB_VIEW_NOT_READY;
    if (vm->fb_width == 0u || vm->fb_width > 1024u || vm->fb_height == 0u ||
        vm->fb_height > 1024u)
        return HAEUN_FB_VIEW_BAD_ARGUMENT;
    *w = vm->fb_width;
    *h = vm->fb_height;
    *base = vm->fb_base;
    *bytes = (uint64_t)(*w) * (uint64_t)(*h) * 4u;
    end = (uint64_t)(*base) + *bytes;
    if (end < (uint64_t)(*base) || end > (uint64_t)vm->ram_size)
        return HAEUN_FB_VIEW_OOB;
    return HAEUN_FB_VIEW_OK;
}

/**
 * Snapshot guest FB into BGRA for host blitting (no guest mutation).
 */
haeun_fb_view_result haeun_fb_view_snapshot_bgra(const haeun_vm *vm,
                                                 uint8_t *out_bgra,
                                                 size_t out_cap, uint32_t *out_w,
                                                 uint32_t *out_h) {
    uint32_t w = 0u, h = 0u, base = 0u;
    uint64_t bytes = 0u;
    uint32_t i;
    const uint8_t *src;
    haeun_fb_view_result st = span(vm, &w, &h, &base, &bytes);
    if (st != HAEUN_FB_VIEW_OK)
        return st;
    if (out_bgra == NULL || out_w == NULL || out_h == NULL)
        return HAEUN_FB_VIEW_BAD_ARGUMENT;
    if (out_cap < bytes)
        return HAEUN_FB_VIEW_NOSPACE;
    src = vm->ram + (size_t)base;
    for (i = 0u; i < w * h; ++i) {
        const uint8_t *p = src + (size_t)i * 4u;
        uint8_t *d = out_bgra + (size_t)i * 4u;
        d[0] = p[2]; /* B */
        d[1] = p[1]; /* G */
        d[2] = p[0]; /* R */
        d[3] = p[3]; /* A */
    }
    *out_w = w;
    *out_h = h;
    return HAEUN_FB_VIEW_OK;
}

/**
 * Centers the scaled FB in the client area and maps clicks into FB pixels.
 */
int haeun_fb_view_map_mouse(int client_x, int client_y, int win_w, int win_h,
                            uint32_t fb_w, uint32_t fb_h, int32_t *fb_x,
                            int32_t *fb_y) {
    int scale;
    int draw_w;
    int draw_h;
    int ox;
    int oy;
    int lx;
    int ly;
    if (fb_x == NULL || fb_y == NULL || fb_w == 0u || fb_h == 0u || win_w <= 0 ||
        win_h <= 0)
        return -1;
    scale = win_w / (int)fb_w;
    if (win_h / (int)fb_h < scale)
        scale = win_h / (int)fb_h;
    if (scale < 1)
        scale = 1;
    draw_w = (int)fb_w * scale;
    draw_h = (int)fb_h * scale;
    ox = (win_w - draw_w) / 2;
    oy = (win_h - draw_h) / 2;
    lx = client_x - ox;
    ly = client_y - oy;
    if (lx < 0 || ly < 0 || lx >= draw_w || ly >= draw_h)
        return -1;
    *fb_x = (int32_t)(lx / scale);
    *fb_y = (int32_t)(ly / scale);
    return 0;
}
