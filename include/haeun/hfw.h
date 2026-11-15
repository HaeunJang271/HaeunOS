#ifndef HAEUN_HFW_H
#define HAEUN_HFW_H

#include <stddef.h>
#include <stdint.h>

#define HAEUN_HFW_HEADER_SIZE 32u
#define HAEUN_HFW_MAX_FIRMWARE_SIZE 65536u

typedef enum {
    HAEUN_HFW_OK = 0,
    HAEUN_HFW_BAD_ARGUMENT,
    HAEUN_HFW_INVALID_IMAGE,
    HAEUN_HFW_UNSUPPORTED_VERSION,
    HAEUN_HFW_RAM_TOO_SMALL
} haeun_hfw_result;

/* Validates HFW v1 and stages HAEUN guest bytes at RAM address zero.
 * Image and RAM must be disjoint host memory ranges. RAM is unchanged on error. */
haeun_hfw_result haeun_hfw_load(const uint8_t *image, size_t image_size,
                                uint8_t *ram, size_t ram_size,
                                uint32_t *entry_address, uint32_t *isa_version);

#endif
