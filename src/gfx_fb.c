#include "haeun/gfx.h"
#include "haeun/board_mmio.h"

#include <string.h>

/**
 * Programs FB MMIO and marks gfx ready only when STATUS reports OK.
 */
haeun_gfx_result haeun_gfx_fb_enable(haeun_gfx *gfx, haeun_vm *vm,
                                     uint32_t width, uint32_t height,
                                     uint32_t base) {
    uint64_t bytes;
    uint64_t end;
    if (gfx == NULL || vm == NULL || width == 0u || width > 1024u ||
        height == 0u || height > 1024u)
        return HAEUN_GFX_BAD_ARGUMENT;
    bytes = (uint64_t)width * (uint64_t)height * 4u;
    end = (uint64_t)base + bytes;
    if (end < (uint64_t)base || end > (uint64_t)vm->ram_size)
        return HAEUN_GFX_BAD_ARGUMENT;
    memset(gfx, 0, sizeof(*gfx));
    gfx->vm = vm;
    gfx->width = width;
    gfx->height = height;
    gfx->base = base;
    haeun_board_write(vm, HAEUN_FB_BASE + 8u, width);
    haeun_board_write(vm, HAEUN_FB_BASE + 12u, height);
    haeun_board_write(vm, HAEUN_FB_BASE + 16u, base);
    haeun_board_write(vm, HAEUN_FB_BASE + 4u, 1u);
    if (haeun_board_read(vm, HAEUN_FB_BASE) != 1u)
        return HAEUN_GFX_NOT_READY;
    gfx->ready = 1;
    return HAEUN_GFX_OK;
}

/**
 * Clears CONTROL.enable and the local ready flag.
 */
haeun_gfx_result haeun_gfx_fb_disable(haeun_gfx *gfx) {
    if (gfx == NULL || gfx->vm == NULL) return HAEUN_GFX_BAD_ARGUMENT;
    haeun_board_write(gfx->vm, HAEUN_FB_BASE + 4u, 0u);
    gfx->ready = 0;
    return HAEUN_GFX_OK;
}

/**
 * Returns non-zero when the driver and board STATUS agree the FB is live.
 */
int haeun_gfx_fb_ready(const haeun_gfx *gfx) {
    if (gfx == NULL || gfx->vm == NULL || !gfx->ready) return 0;
    return haeun_board_read(gfx->vm, HAEUN_FB_BASE) == 1u ? 1 : 0;
}
