/**
 * @file cp_hash.c
 * @brief Implementation of FNV-1a, fast Hash64, and standard SHA-256.
 */

#include "cplus/cp_hash.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Non-Cryptographic Fast Hashes
 * ------------------------------------------------------------------------- */
u32 cp_hash_fnv1a32(const void *data, usize len) {
    if (!data) return 0;
    const u8 *p = (const u8 *)data;
    u32 hash = 0x811c9dc5u;
    for (usize i = 0; i < len; ++i) {
        hash ^= p[i];
        hash *= 0x01000193u;
    }
    return hash;
}

u64 cp_hash_fnv1a64(const void *data, usize len) {
    if (!data) return 0;
    const u8 *p = (const u8 *)data;
    u64 hash = 0xcbf29ce484222325ULL;
    for (usize i = 0; i < len; ++i) {
        hash ^= p[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

u64 cp_hash64(const void *data, usize len) {
    if (!data) return 0;
    const u8 *p = (const u8 *)data;
    u64 h = 0x243f6a8885a308d3ULL ^ (len * 0x9e3779b97f4a7c15ULL);

    while (len >= 8) {
        u64 k;
        memcpy(&k, p, 8);
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 33;
        h ^= k;
        h *= 0x9ddfea08eb382d69ULL;
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        u64 k = 0;
        memcpy(&k, p, len);
        h ^= k;
        h *= 0x9ddfea08eb382d69ULL;
    }

    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h;
}

/* -------------------------------------------------------------------------
 * SHA-256 Implementation (FIPS 180-4)
 * ------------------------------------------------------------------------- */
static const u32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define GAMMA0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define GAMMA1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static void cp_sha256_transform(cp_sha256_ctx_t *ctx, const u8 block[64]) {
    u32 w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = ((u32)block[i * 4] << 24) |
               ((u32)block[i * 4 + 1] << 16) |
               ((u32)block[i * 4 + 2] << 8) |
               ((u32)block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        w[i] = GAMMA1(w[i - 2]) + w[i - 7] + GAMMA0(w[i - 15]) + w[i - 16];
    }

    u32 a = ctx->state[0];
    u32 b = ctx->state[1];
    u32 c = ctx->state[2];
    u32 d = ctx->state[3];
    u32 e = ctx->state[4];
    u32 f = ctx->state[5];
    u32 g = ctx->state[6];
    u32 h = ctx->state[7];

    for (int i = 0; i < 64; ++i) {
        u32 t1 = h + SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        u32 t2 = SIGMA0(a) + MAJ(a, b, c);
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

void cp_sha256_init(cp_sha256_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

void cp_sha256_update(cp_sha256_ctx_t *ctx, const void *data, usize len) {
    if (!ctx || !data || len == 0) return;
    const u8 *p = (const u8 *)data;
    usize buf_idx = (usize)(ctx->count & 0x3F);
    ctx->count += len;

    if (buf_idx > 0) {
        usize space = 64 - buf_idx;
        if (len < space) {
            memcpy(ctx->buffer + buf_idx, p, len);
            return;
        }
        memcpy(ctx->buffer + buf_idx, p, space);
        cp_sha256_transform(ctx, ctx->buffer);
        p += space;
        len -= space;
    }

    while (len >= 64) {
        cp_sha256_transform(ctx, p);
        p += 64;
        len -= 64;
    }

    if (len > 0) {
        memcpy(ctx->buffer, p, len);
    }
}

void cp_sha256_final(cp_sha256_ctx_t *ctx, u8 out_hash[32]) {
    if (!ctx || !out_hash) return;
    u64 total_bits = ctx->count * 8;
    usize buf_idx = (usize)(ctx->count & 0x3F);

    ctx->buffer[buf_idx++] = 0x80;
    if (buf_idx > 56) {
        memset(ctx->buffer + buf_idx, 0, 64 - buf_idx);
        cp_sha256_transform(ctx, ctx->buffer);
        buf_idx = 0;
    }
    memset(ctx->buffer + buf_idx, 0, 56 - buf_idx);

    for (int i = 7; i >= 0; --i) {
        ctx->buffer[56 + (7 - i)] = (u8)((total_bits >> (i * 8)) & 0xFF);
    }
    cp_sha256_transform(ctx, ctx->buffer);

    for (int i = 0; i < 8; ++i) {
        out_hash[i * 4]     = (u8)((ctx->state[i] >> 24) & 0xFF);
        out_hash[i * 4 + 1] = (u8)((ctx->state[i] >> 16) & 0xFF);
        out_hash[i * 4 + 2] = (u8)((ctx->state[i] >> 8) & 0xFF);
        out_hash[i * 4 + 3] = (u8)(ctx->state[i] & 0xFF);
    }
}

void cp_sha256(const void *data, usize len, u8 out_hash[32]) {
    cp_sha256_ctx_t ctx;
    cp_sha256_init(&ctx);
    cp_sha256_update(&ctx, data, len);
    cp_sha256_final(&ctx, out_hash);
}

void cp_sha256_hex(const u8 in_hash[32], char out_hex[65]) {
    if (!in_hash || !out_hex) return;
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 32; ++i) {
        out_hex[i * 2]     = hex_chars[(in_hash[i] >> 4) & 0x0F];
        out_hex[i * 2 + 1] = hex_chars[in_hash[i] & 0x0F];
    }
    out_hex[64] = '\0';
}
