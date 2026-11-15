#include "haeun/fb_dump.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * Validates FB control state and returns the guest RAM pixel span.
 */
static haeun_fb_dump_result fb_span(const haeun_vm *vm, uint32_t *width,
                                    uint32_t *height, uint32_t *base,
                                    uint64_t *bytes) {
    uint64_t end;
    if (vm == NULL || vm->ram == NULL || width == NULL || height == NULL ||
        base == NULL || bytes == NULL)
        return HAEUN_FB_DUMP_BAD_ARGUMENT;
    if ((vm->fb_control & 1u) == 0u)
        return HAEUN_FB_DUMP_NOT_READY;
    if (vm->fb_width == 0u || vm->fb_width > 1024u || vm->fb_height == 0u ||
        vm->fb_height > 1024u)
        return HAEUN_FB_DUMP_BAD_ARGUMENT;
    *width = vm->fb_width;
    *height = vm->fb_height;
    *base = vm->fb_base;
    *bytes = (uint64_t)(*width) * (uint64_t)(*height) * 4u;
    end = (uint64_t)(*base) + *bytes;
    if (end < (uint64_t)(*base) || end > (uint64_t)vm->ram_size)
        return HAEUN_FB_DUMP_OOB;
    return HAEUN_FB_DUMP_OK;
}

/**
 * Emits one P6 framebuffer snapshot from guest RAM at BASE.
 */
haeun_fb_dump_result haeun_fb_dump_ppm(const haeun_vm *vm, FILE *out) {
    uint32_t width = 0u;
    uint32_t height = 0u;
    uint32_t base = 0u;
    uint64_t bytes = 0u;
    uint32_t y;
    uint32_t x;
    haeun_fb_dump_result st;
    const uint8_t *pix;
    if (out == NULL)
        return HAEUN_FB_DUMP_BAD_ARGUMENT;
    st = fb_span(vm, &width, &height, &base, &bytes);
    if (st != HAEUN_FB_DUMP_OK)
        return st;
    if (fprintf(out, "P6\n%u %u\n255\n", width, height) < 0)
        return HAEUN_FB_DUMP_IO;
    pix = vm->ram + (size_t)base;
    for (y = 0u; y < height; ++y) {
        for (x = 0u; x < width; ++x) {
            size_t off = ((size_t)y * (size_t)width + (size_t)x) * 4u;
            /* RGBA8888 packed LE: emit R,G,B; skip A. */
            if (fwrite(pix + off, 1u, 3u, out) != 3u)
                return HAEUN_FB_DUMP_IO;
        }
    }
    if (fflush(out) != 0)
        return HAEUN_FB_DUMP_IO;
    return HAEUN_FB_DUMP_OK;
}

/**
 * Dumps the framebuffer to a new binary file at path.
 */
haeun_fb_dump_result haeun_fb_dump_ppm_path(const haeun_vm *vm,
                                            const char *path) {
    FILE *file;
    haeun_fb_dump_result st;
    if (path == NULL || path[0] == '\0')
        return HAEUN_FB_DUMP_BAD_ARGUMENT;
    file = fopen(path, "wb");
    if (file == NULL)
        return HAEUN_FB_DUMP_IO;
    st = haeun_fb_dump_ppm(vm, file);
    if (fclose(file) != 0 && st == HAEUN_FB_DUMP_OK)
        return HAEUN_FB_DUMP_IO;
    return st;
}
