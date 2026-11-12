/**
 * @file cp_hash.h
 * @brief Ultra-fast non-cryptographic (FNV-1a, Hash64) and cryptographic (SHA-256) hash functions.
 */

#ifndef CPLUS_HASH_H
#define CPLUS_HASH_H

#include "cp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Non-Cryptographic Fast Hashes
 * ------------------------------------------------------------------------- */
/**
 * @brief 32-bit FNV-1a hash.
 */
u32 cp_hash_fnv1a32(const void *data, usize len);

/**
 * @brief 64-bit FNV-1a hash.
 */
u64 cp_hash_fnv1a64(const void *data, usize len);

/**
 * @brief Blindingly fast 64-bit non-cryptographic hash (< 1 ns/byte).
 */
u64 cp_hash64(const void *data, usize len);

/* -------------------------------------------------------------------------
 * Cryptographic Hash: SHA-256 (FIPS 180-4 Standard)
 * ------------------------------------------------------------------------- */
typedef struct {
    u32 state[8];
    u64 count;
    u8 buffer[64];
} cp_sha256_ctx_t;

void cp_sha256_init(cp_sha256_ctx_t *ctx);
void cp_sha256_update(cp_sha256_ctx_t *ctx, const void *data, usize len);
void cp_sha256_final(cp_sha256_ctx_t *ctx, u8 out_hash[32]);

/**
 * @brief One-liner compute SHA-256 hash.
 */
void cp_sha256(const void *data, usize len, u8 out_hash[32]);

/**
 * @brief Format 32-byte SHA-256 hash as 64-character lowercase hexadecimal string (null-terminated).
 */
void cp_sha256_hex(const u8 in_hash[32], char out_hex[65]);

/* -------------------------------------------------------------------------
 * PascalCase API
 * ------------------------------------------------------------------------- */
#define HashFnv1a32(data, len)          cp_hash_fnv1a32(data, len)
#define HashFnv1a64(data, len)          cp_hash_fnv1a64(data, len)
#define Hash64(data, len)               cp_hash64(data, len)

#define Sha256(data, len, out)          cp_sha256(data, len, out)
#define Sha256Hex(in_hash, out_hex)     cp_sha256_hex(in_hash, out_hex)
#define Sha256Init(ctx)                 cp_sha256_init(ctx)
#define Sha256Update(ctx, data, len)    cp_sha256_update(ctx, data, len)
#define Sha256Final(ctx, out)           cp_sha256_final(ctx, out)

/* -------------------------------------------------------------------------
 * Dot-Notation Namespace
 * ------------------------------------------------------------------------- */
typedef struct {
    u32 (*const fnv1a32)(const void *data, usize len);
    u64 (*const fnv1a64)(const void *data, usize len);
    u64 (*const u64_hash)(const void *data, usize len);
    void (*const sha256)(const void *data, usize len, u8 out_hash[32]);
    void (*const sha256Hex)(const u8 in_hash[32], char out_hex[65]);
} cp_hash_namespace_t;

static const cp_hash_namespace_t hash_api = {
    .fnv1a32 = cp_hash_fnv1a32,
    .fnv1a64 = cp_hash_fnv1a64,
    .u64_hash = cp_hash64,
    .sha256 = cp_sha256,
    .sha256Hex = cp_sha256_hex,
};

#ifdef __cplusplus
}
#endif

#endif /* CPLUS_HASH_H */
