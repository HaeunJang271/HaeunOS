#include "haeun/sec.h"

#include <stdio.h>
#include <string.h>

/**
 * Writes a security audit line; used on every deny path.
 */
void haeun_sec_log(const char *event, const char *detail) {
    const char *e = event != NULL ? event : "(null)";
    const char *d = detail != NULL ? detail : "";
    fprintf(stderr, "[haeun-sec] failure: %s %s\n", e, d);
}

/**
 * Denies when the caller is not in supervisor mode.
 */
haeun_sec_result haeun_sec_require_supervisor(uint32_t privilege) {
    if (privilege != HAEUN_PRIV_SUPERVISOR) {
        haeun_sec_log("privilege", "supervisor required");
        return HAEUN_SEC_DENIED;
    }
    return HAEUN_SEC_OK;
}

/**
 * Allows only the documented user syscall numbers; logs unknowns.
 */
haeun_sec_result haeun_sec_validate_syscall(uint32_t privilege,
                                            uint32_t number) {
    (void)privilege;
    if (number > (uint32_t)HAEUN_SYS_MAX) {
        haeun_sec_log("syscall", "unknown number");
        return HAEUN_SEC_DENIED;
    }
    return HAEUN_SEC_OK;
}

/**
 * Bounds-checks a user buffer; supervisor may touch any range (kernel path).
 */
haeun_sec_result haeun_sec_check_user_range(uint32_t privilege, uint32_t addr,
                                            uint32_t length, uint32_t lo,
                                            uint32_t hi) {
    uint64_t end;
    if (privilege == HAEUN_PRIV_SUPERVISOR) return HAEUN_SEC_OK;
    if (hi < lo) {
        haeun_sec_log("user_range", "invalid window");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    end = (uint64_t)addr + (uint64_t)length;
    if (end < (uint64_t)addr || addr < lo || end > (uint64_t)hi) {
        haeun_sec_log("user_range", "pointer outside allowed region");
        return HAEUN_SEC_DENIED;
    }
    return HAEUN_SEC_OK;
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/**
 * Checks HXF magic, minimum size, ISA version 1..4, and aligned entry.
 */
haeun_sec_result haeun_sec_validate_exec(const uint8_t *image, size_t size) {
    uint32_t magic;
    uint32_t isa;
    uint32_t entry;
    if (image == NULL || size < 32u) {
        haeun_sec_log("exec", "truncated or null image");
        return HAEUN_SEC_DENIED;
    }
    magic = rd32(image);
    (void)magic;
    /* Magic is ASCII "HXF1". */
    if (image[0] != 'H' || image[1] != 'X' || image[2] != 'F' ||
        image[3] != '1') {
        haeun_sec_log("exec", "bad magic");
        return HAEUN_SEC_DENIED;
    }
    /* Layout matches existing HXF header: isa at +8, entry at +16 (see hxf.c). */
    isa = rd32(image + 8u);
    entry = rd32(image + 16u);
    if (isa < 1u || isa > 4u) {
        haeun_sec_log("exec", "unsupported ISA");
        return HAEUN_SEC_DENIED;
    }
    if ((entry & 3u) != 0u) {
        haeun_sec_log("exec", "misaligned entry");
        return HAEUN_SEC_DENIED;
    }
    return HAEUN_SEC_OK;
}
