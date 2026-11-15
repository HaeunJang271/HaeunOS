#ifndef HAEUN_SEC_BOOT_H
#define HAEUN_SEC_BOOT_H

#include "haeun/sec.h"
#include "haeun/sha256.h"

#include <stddef.h>
#include <stdint.h>

/**
 * Parses a 64-character lowercase/uppercase hex SHA-256 into out[32].
 */
haeun_sec_result haeun_sec_boot_parse_root_hash(
    const char *hex64, uint8_t out[HAEUN_SHA256_DIGEST_LEN]);

/**
 * Measures a validated HXF image: runs haeun_sec_validate_exec then SHA-256
 * over the full image bytes into out[32].
 */
haeun_sec_result haeun_sec_boot_measure(const uint8_t *image, size_t size,
                                        uint8_t out[HAEUN_SHA256_DIGEST_LEN]);

/**
 * Measures image and compares to expected root hash; logs on mismatch.
 */
haeun_sec_result haeun_sec_boot_verify(const uint8_t *image, size_t size,
                                       const uint8_t expected[HAEUN_SHA256_DIGEST_LEN]);

/**
 * Accepts the image if its digest matches any of n root hashes packed as
 * n*32 contiguous bytes (n may be 0 → deny).
 */
haeun_sec_result haeun_sec_boot_verify_allowlist(const uint8_t *image,
                                                 size_t size,
                                                 const uint8_t *roots,
                                                 size_t n);

/**
 * Verifies the HXF at LBA 0 of a raw disk (header file_size bytes) against
 * expected. Used before firmware disk boot when a root hash is provisioned.
 */
haeun_sec_result haeun_sec_boot_verify_disk_hxf(
    const uint8_t *disk, size_t disk_size,
    const uint8_t expected[HAEUN_SHA256_DIGEST_LEN]);

#endif

