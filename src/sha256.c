#include "haeun/sha256.h"

#include <string.h>

static uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32u - n));
}

static void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t load_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void transform(haeun_sha256_ctx *ctx, const uint8_t block[64]) {
    static const uint32_t k[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
        0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
        0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
        0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
        0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
        0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
        0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
        0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
        0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
        0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
        0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h, t1, t2, i;
    for (i = 0u; i < 16u; ++i) w[i] = load_be32(block + i * 4u);
    for (i = 16u; i < 64u; ++i) {
        uint32_t s0 = rotr(w[i - 15u], 7u) ^ rotr(w[i - 15u], 18u) ^
                      (w[i - 15u] >> 3u);
        uint32_t s1 = rotr(w[i - 2u], 17u) ^ rotr(w[i - 2u], 19u) ^
                      (w[i - 2u] >> 10u);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    for (i = 0u; i < 64u; ++i) {
        uint32_t S1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t S0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        t1 = h + S1 + ch + k[i] + w[i];
        t2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/**
 * Resets the SHA-256 chaining values and buffer.
 */
void haeun_sha256_init(haeun_sha256_ctx *ctx) {
    if (ctx == NULL) return;
    ctx->state[0] = 0x6a09e667u;
    ctx->state[1] = 0xbb67ae85u;
    ctx->state[2] = 0x3c6ef372u;
    ctx->state[3] = 0xa54ff53au;
    ctx->state[4] = 0x510e527fu;
    ctx->state[5] = 0x9b05688cu;
    ctx->state[6] = 0x1f83d9abu;
    ctx->state[7] = 0x5be0cd19u;
    ctx->bitlen = 0u;
    ctx->buflen = 0u;
}

/**
 * Feeds len bytes into the hash; ignores NULL data when len is zero.
 */
void haeun_sha256_update(haeun_sha256_ctx *ctx, const uint8_t *data,
                         size_t len) {
    size_t i;
    if (ctx == NULL || (data == NULL && len != 0u)) return;
    for (i = 0u; i < len; ++i) {
        ctx->buffer[ctx->buflen++] = data[i];
        if (ctx->buflen == 64u) {
            transform(ctx, ctx->buffer);
            ctx->bitlen += 512u;
            ctx->buflen = 0u;
        }
    }
}

/**
 * Pads and emits the 32-byte digest.
 */
void haeun_sha256_final(haeun_sha256_ctx *ctx,
                        uint8_t out[HAEUN_SHA256_DIGEST_LEN]) {
    uint32_t i;
    uint64_t bits;
    if (ctx == NULL || out == NULL) return;
    i = ctx->buflen;
    ctx->buffer[i++] = 0x80u;
    if (i > 56u) {
        while (i < 64u) ctx->buffer[i++] = 0u;
        transform(ctx, ctx->buffer);
        i = 0u;
    }
    while (i < 56u) ctx->buffer[i++] = 0u;
    bits = ctx->bitlen + (uint64_t)ctx->buflen * 8u;
    store_be32(ctx->buffer + 56, (uint32_t)(bits >> 32));
    store_be32(ctx->buffer + 60, (uint32_t)bits);
    transform(ctx, ctx->buffer);
    for (i = 0u; i < 8u; ++i) store_be32(out + i * 4u, ctx->state[i]);
}

/**
 * Computes SHA-256 of a contiguous buffer.
 */
void haeun_sha256(const uint8_t *data, size_t len,
                  uint8_t out[HAEUN_SHA256_DIGEST_LEN]) {
    haeun_sha256_ctx ctx;
    haeun_sha256_init(&ctx);
    haeun_sha256_update(&ctx, data, len);
    haeun_sha256_final(&ctx, out);
}
