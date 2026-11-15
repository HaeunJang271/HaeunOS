#include "haeun/hxf.h"
#include "haeun/sec_boot.h"
#include "haeun/sha256.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                               \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);       \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * SHA-256 of empty string matches the published digest.
 */
static int test_sha256_empty(void) {
    uint8_t out[HAEUN_SHA256_DIGEST_LEN];
    static const uint8_t expect[HAEUN_SHA256_DIGEST_LEN] = {
        0xe3u, 0xb0u, 0xc4u, 0x42u, 0x98u, 0xfcu, 0x1cu, 0x14u, 0x9au, 0xfbu,
        0xf4u, 0xc8u, 0x99u, 0x6fu, 0xb9u, 0x24u, 0x27u, 0xaeu, 0x41u, 0xe4u,
        0x64u, 0x9bu, 0x93u, 0x4cu, 0xa4u, 0x95u, 0x99u, 0x1bu, 0x78u, 0x52u,
        0xb8u, 0x55u};
    haeun_sha256(NULL, 0u, out);
    CHECK(memcmp(out, expect, HAEUN_SHA256_DIGEST_LEN) == 0);
    return 0;
}

/**
 * SHA-256 of "abc" matches the published digest.
 */
static int test_sha256_abc(void) {
    uint8_t out[HAEUN_SHA256_DIGEST_LEN];
    static const uint8_t expect[HAEUN_SHA256_DIGEST_LEN] = {
        0xbau, 0x78u, 0x16u, 0xbfu, 0x8fu, 0x01u, 0xcfu, 0xeau, 0x41u, 0x41u,
        0x40u, 0xdeu, 0x5du, 0xaeu, 0x22u, 0x23u, 0xb0u, 0x03u, 0x61u, 0xa3u,
        0x96u, 0x17u, 0x7au, 0x9cu, 0xb4u, 0x10u, 0xffu, 0x61u, 0xf2u, 0x00u,
        0x15u, 0xadu};
    haeun_sha256((const uint8_t *)"abc", 3u, out);
    CHECK(memcmp(out, expect, HAEUN_SHA256_DIGEST_LEN) == 0);
    return 0;
}

static int make_hxf(uint8_t *image, size_t cap, size_t *out_size) {
    uint8_t seg[4] = {0x00, 0x00, 0x00, 0x04};
    haeun_hxf_segment s;
    memset(&s, 0, sizeof(s));
    s.flags = 5u;
    s.virtual_address = 0x1000u;
    s.file_data = seg;
    s.file_size = 4u;
    s.memory_size = 8u;
    s.alignment = 4u;
    return haeun_hxf_write(&s, 1u, 0x1000u, image, cap, out_size) ==
                   HAEUN_HXF_OK
               ? 0
               : 1;
}

/**
 * Hex root-hash parsing accepts 64 digits and rejects short strings.
 */
static int test_parse_root_hash(void) {
    uint8_t out[HAEUN_SHA256_DIGEST_LEN];
    uint8_t expect[HAEUN_SHA256_DIGEST_LEN];
    char hex[65];
    size_t i;
    memset(expect, 0, sizeof(expect));
    expect[0] = 0xabu;
    expect[31] = 0xedu;
    for (i = 0u; i < HAEUN_SHA256_DIGEST_LEN; ++i)
        (void)sprintf(hex + i * 2u, "%02x", expect[i]);
    CHECK(haeun_sec_boot_parse_root_hash(hex, out) == HAEUN_SEC_OK);
    CHECK(memcmp(out, expect, HAEUN_SHA256_DIGEST_LEN) == 0);
    CHECK(haeun_sec_boot_parse_root_hash("abcd", out) == HAEUN_SEC_BAD_ARGUMENT);
    return 0;
}

/**
 * Measure + verify accept matching root; deny wrong hash and garbage image.
 */
static int test_boot_verify(void) {
    uint8_t image[256];
    uint8_t dig[HAEUN_SHA256_DIGEST_LEN];
    uint8_t wrong[HAEUN_SHA256_DIGEST_LEN];
    uint8_t bad[64];
    size_t out_size = 0u;
    uint8_t roots[HAEUN_SHA256_DIGEST_LEN * 2u];
    memset(image, 0, sizeof(image));
    memset(bad, 0, sizeof(bad));
    CHECK(make_hxf(image, sizeof(image), &out_size) == 0);
    CHECK(haeun_sec_boot_measure(image, out_size, dig) == HAEUN_SEC_OK);
    CHECK(haeun_sec_boot_verify(image, out_size, dig) == HAEUN_SEC_OK);
    memset(wrong, 0xa5, sizeof(wrong));
    CHECK(haeun_sec_boot_verify(image, out_size, wrong) == HAEUN_SEC_DENIED);
    CHECK(haeun_sec_boot_measure(bad, sizeof(bad), dig) == HAEUN_SEC_DENIED);
    memcpy(roots, wrong, HAEUN_SHA256_DIGEST_LEN);
    CHECK(haeun_sec_boot_measure(image, out_size, dig) == HAEUN_SEC_OK);
    memcpy(roots + HAEUN_SHA256_DIGEST_LEN, dig, HAEUN_SHA256_DIGEST_LEN);
    CHECK(haeun_sec_boot_verify_allowlist(image, out_size, roots, 2u) ==
          HAEUN_SEC_OK);
    CHECK(haeun_sec_boot_verify_allowlist(image, out_size, roots, 1u) ==
          HAEUN_SEC_DENIED);
    CHECK(haeun_sec_boot_verify_allowlist(image, out_size, roots, 0u) ==
          HAEUN_SEC_DENIED);
    return 0;
}

/**
 * Disk image HXF at LBA 0 is verified using declared file_size.
 */
static int test_boot_verify_disk(void) {
    uint8_t image[256];
    uint8_t disk[512];
    uint8_t dig[HAEUN_SHA256_DIGEST_LEN];
    uint8_t wrong[HAEUN_SHA256_DIGEST_LEN];
    size_t out_size = 0u;
    memset(image, 0, sizeof(image));
    memset(disk, 0, sizeof(disk));
    CHECK(make_hxf(image, sizeof(image), &out_size) == 0);
    CHECK(out_size <= sizeof(disk));
    memcpy(disk, image, out_size);
    CHECK(haeun_sec_boot_measure(image, out_size, dig) == HAEUN_SEC_OK);
    CHECK(haeun_sec_boot_verify_disk_hxf(disk, sizeof(disk), dig) ==
          HAEUN_SEC_OK);
    memset(wrong, 0x5a, sizeof(wrong));
    CHECK(haeun_sec_boot_verify_disk_hxf(disk, sizeof(disk), wrong) ==
          HAEUN_SEC_DENIED);
    return 0;
}

int main(void) {
    if (test_sha256_empty() != 0) return 1;
    if (test_sha256_abc() != 0) return 1;
    if (test_parse_root_hash() != 0) return 1;
    if (test_boot_verify() != 0) return 1;
    if (test_boot_verify_disk() != 0) return 1;
    puts("all sec boot tests passed");
    return 0;
}
