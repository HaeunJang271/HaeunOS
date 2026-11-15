#include "haeun/sec_boot.h"

#include <string.h>

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (int)(c - 'A') + 10;
    return -1;
}

/**
 * Decodes a 64-digit hex root hash string into 32 bytes.
 */
haeun_sec_result haeun_sec_boot_parse_root_hash(
    const char *hex64, uint8_t out[HAEUN_SHA256_DIGEST_LEN]) {
    size_t i;
    if (hex64 == NULL || out == NULL) {
        haeun_sec_log("boot_parse", "null argument");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    if (strlen(hex64) != (size_t)(HAEUN_SHA256_DIGEST_LEN * 2u)) {
        haeun_sec_log("boot_parse", "expected 64 hex digits");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    for (i = 0u; i < HAEUN_SHA256_DIGEST_LEN; ++i) {
        int hi = hex_nibble(hex64[i * 2u]);
        int lo = hex_nibble(hex64[i * 2u + 1u]);
        if (hi < 0 || lo < 0) {
            haeun_sec_log("boot_parse", "non-hex digit");
            return HAEUN_SEC_BAD_ARGUMENT;
        }
        out[i] = (uint8_t)(((unsigned)hi << 4) | (unsigned)lo);
    }
    return HAEUN_SEC_OK;
}

/**
 * Validates then digests an HXF image for secure-boot measurement.
 */
haeun_sec_result haeun_sec_boot_measure(const uint8_t *image, size_t size,
                                        uint8_t out[HAEUN_SHA256_DIGEST_LEN]) {
    haeun_sec_result r;
    if (out == NULL) {
        haeun_sec_log("boot_measure", "null digest out");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    r = haeun_sec_validate_exec(image, size);
    if (r != HAEUN_SEC_OK) return r;
    haeun_sha256(image, size, out);
    return HAEUN_SEC_OK;
}

/**
 * Verifies an image against a single provisioned root hash.
 */
haeun_sec_result haeun_sec_boot_verify(const uint8_t *image, size_t size,
                                       const uint8_t expected[HAEUN_SHA256_DIGEST_LEN]) {
    uint8_t dig[HAEUN_SHA256_DIGEST_LEN];
    haeun_sec_result r;
    if (expected == NULL) {
        haeun_sec_log("boot_verify", "null expected hash");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    r = haeun_sec_boot_measure(image, size, dig);
    if (r != HAEUN_SEC_OK) return r;
    if (memcmp(dig, expected, HAEUN_SHA256_DIGEST_LEN) != 0) {
        haeun_sec_log("boot_verify", "root hash mismatch");
        return HAEUN_SEC_DENIED;
    }
    return HAEUN_SEC_OK;
}

/**
 * Verifies an image against a packed list of acceptable root hashes.
 */
haeun_sec_result haeun_sec_boot_verify_allowlist(const uint8_t *image,
                                                 size_t size,
                                                 const uint8_t *roots,
                                                 size_t n) {
    uint8_t dig[HAEUN_SHA256_DIGEST_LEN];
    haeun_sec_result r;
    size_t i;
    if (n > 0u && roots == NULL) {
        haeun_sec_log("boot_allowlist", "null roots");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    r = haeun_sec_boot_measure(image, size, dig);
    if (r != HAEUN_SEC_OK) return r;
    for (i = 0u; i < n; ++i) {
        if (memcmp(dig, roots + i * HAEUN_SHA256_DIGEST_LEN,
                   HAEUN_SHA256_DIGEST_LEN) == 0)
            return HAEUN_SEC_OK;
    }
    haeun_sec_log("boot_allowlist", "no matching root hash");
    return HAEUN_SEC_DENIED;
}

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/**
 * Pulls the HXF spanning LBA 0 from a raw disk and verifies its root hash.
 */
haeun_sec_result haeun_sec_boot_verify_disk_hxf(
    const uint8_t *disk, size_t disk_size,
    const uint8_t expected[HAEUN_SHA256_DIGEST_LEN]) {
    uint32_t declared;
    if (disk == NULL || expected == NULL) {
        haeun_sec_log("boot_disk", "null argument");
        return HAEUN_SEC_BAD_ARGUMENT;
    }
    if (disk_size < 32u || (disk_size % 512u) != 0u) {
        haeun_sec_log("boot_disk", "bad disk size");
        return HAEUN_SEC_DENIED;
    }
    if (disk[0] != 'H' || disk[1] != 'X' || disk[2] != 'F' || disk[3] != '1') {
        haeun_sec_log("boot_disk", "missing HXF magic");
        return HAEUN_SEC_DENIED;
    }
    declared = rd32(disk + 28u);
    if (declared < 32u || (uint64_t)declared > (uint64_t)disk_size) {
        haeun_sec_log("boot_disk", "declared size out of range");
        return HAEUN_SEC_DENIED;
    }
    return haeun_sec_boot_verify(disk, (size_t)declared, expected);
}
