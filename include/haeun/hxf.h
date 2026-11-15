#ifndef HAEUN_HXF_H
#define HAEUN_HXF_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAEUN_HXF_OK = 0,
    HAEUN_HXF_BAD_ARGUMENT,
    HAEUN_HXF_INVALID_IMAGE,
    HAEUN_HXF_UNSUPPORTED_VERSION,
    HAEUN_HXF_NO_MEMORY,
    HAEUN_HXF_BUFFER_TOO_SMALL
} haeun_hxf_result;

typedef struct {
    uint32_t flags;
    uint32_t virtual_address;
    const uint8_t *file_data;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t alignment;
} haeun_hxf_segment;

/* Legacy v1-only loader. RAM is unchanged on failure. Image and RAM storage
 * must be disjoint host memory ranges. */
haeun_hxf_result haeun_hxf_load(const uint8_t *image, size_t image_size,
                                uint8_t *ram, size_t ram_size,
                                uint32_t *entry_address);
/* Version-aware loader for HXF format v1 with guest ISA 1, 2, or 3. Returns the
 * image ISA version on success. */
haeun_hxf_result haeun_hxf_load_ex(const uint8_t *image, size_t image_size,
                                   uint8_t *ram, size_t ram_size,
                                   uint32_t *entry_address, uint32_t *isa_version);

/* Serializes deterministic HXF v1. Segment data must not overlap output memory.
 * On short output, reports required size and leaves output bytes unchanged. */
haeun_hxf_result haeun_hxf_write(const haeun_hxf_segment *segments,
                                 uint16_t segment_count, uint32_t entry_address,
                                 uint8_t *output, size_t output_capacity,
                                 size_t *output_size);

#endif
