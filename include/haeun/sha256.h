#ifndef HAEUN_SHA256_H
#define HAEUN_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define HAEUN_SHA256_DIGEST_LEN 32u

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t buffer[64];
    uint32_t buflen;
} haeun_sha256_ctx;

/** Initializes a SHA-256 context. */
void haeun_sha256_init(haeun_sha256_ctx *ctx);

/** Absorbs data into the running hash. */
void haeun_sha256_update(haeun_sha256_ctx *ctx, const uint8_t *data, size_t len);

/** Finalizes into a 32-byte digest; ctx must not be reused without init. */
void haeun_sha256_final(haeun_sha256_ctx *ctx, uint8_t out[HAEUN_SHA256_DIGEST_LEN]);

/** One-shot SHA-256 of data into out. */
void haeun_sha256(const uint8_t *data, size_t len,
                  uint8_t out[HAEUN_SHA256_DIGEST_LEN]);

#endif
