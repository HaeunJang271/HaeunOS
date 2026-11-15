#ifndef HAEUN_BOOTINFO_H
#define HAEUN_BOOTINFO_H

#include <stddef.h>
#include <stdint.h>

#define HAEUN_BOOTINFO_VERSION 1u
#define HAEUN_BOOTINFO_SIZE 32u

typedef enum {
    HAEUN_BOOTINFO_OK = 0,
    HAEUN_BOOTINFO_BAD_ARGUMENT,
    HAEUN_BOOTINFO_INVALID,
    HAEUN_BOOTINFO_UNSUPPORTED_VERSION,
    HAEUN_BOOTINFO_BUFFER_TOO_SMALL
} haeun_bootinfo_result;

typedef struct {
    uint32_t flags;
    uint32_t ram_size;
    uint32_t stack_base;
    uint32_t stack_top;
    uint32_t isa_version;
} haeun_bootinfo;

/* Encodes the fixed 32-byte HBI v1 record; no native C struct is serialized. */
haeun_bootinfo_result haeun_bootinfo_encode(uint8_t *output, size_t capacity,
                                             const haeun_bootinfo *info);
/* Validates all fields before writing info. input may contain trailing bytes. */
haeun_bootinfo_result haeun_bootinfo_decode(const uint8_t *input, size_t size,
                                             haeun_bootinfo *info);

#endif
