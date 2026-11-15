#ifndef HAEUN_FB_DUMP_H
#define HAEUN_FB_DUMP_H

#include "haeun/vm.h"

#include <stdio.h>

typedef enum {
    HAEUN_FB_DUMP_OK = 0,
    HAEUN_FB_DUMP_BAD_ARGUMENT,
    HAEUN_FB_DUMP_NOT_READY,
    HAEUN_FB_DUMP_OOB,
    HAEUN_FB_DUMP_IO
} haeun_fb_dump_result;

/**
 * Writes a binary PPM P6 of the guest framebuffer RAM (RGBA8888 → RGB888).
 * Reads WIDTH/HEIGHT/BASE from the VM board state; does not touch guest RAM.
 * Pixels are taken in framebuffer order; each pixel emits bytes 0..2 (R,G,B).
 */
haeun_fb_dump_result haeun_fb_dump_ppm(const haeun_vm *vm, FILE *out);

/**
 * Opens path for binary write and dumps the framebuffer; closes the file.
 */
haeun_fb_dump_result haeun_fb_dump_ppm_path(const haeun_vm *vm,
                                            const char *path);

#endif
