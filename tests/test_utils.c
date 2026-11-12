/**
 * @file test_utils.c
 * @brief Dedicated unit tests for Bit intrinsics, Base64, Fast PRNG, and Colored Logger.
 */

#include "cplus/cplus.h"
#include <string.h>
#include <assert.h>

static void test_bit_intrinsics(void) {
    assert(BitPopcount32(0) == 0);
    assert(BitPopcount32(0xFFFFFFFF) == 32);
    assert(BitPopcount32(0xAA) == 4);
    assert(BitPopcount64(0x8000000000000001ULL) == 2);

    assert(BitClz32(0) == 32);
    assert(BitClz32(1) == 31);
    assert(BitClz32(0x80000000) == 0);
    assert(BitClz64(0) == 64);
    assert(BitClz64(1ULL) == 63);

    assert(BitCtz32(0) == 32);
    assert(BitCtz32(1) == 0);
    assert(BitCtz32(0x80000000) == 31);
    assert(BitCtz64(0) == 64);
    assert(BitCtz64(1ULL) == 0);

    assert(BitBswap16(0x1234) == 0x3412);
    assert(BitBswap32(0x12345678) == 0x78563412);
    assert(BitBswap64(0x0102030405060708ULL) == 0x0807060504030201ULL);

    assert(BitRotl32(1, 1) == 2);
    assert(BitRotr32(1, 1) == 0x80000000);
    assert(BitRotl64(1ULL, 1) == 2ULL);
    assert(BitRotr64(1ULL, 1) == 0x8000000000000000ULL);

    assert(!BitIsPow2(0));
    assert(BitIsPow2(1));
    assert(BitIsPow2(2));
    assert(BitIsPow2(1024));
    assert(!BitIsPow2(1023));

    assert(BitNextPow2(0) == 1);
    assert(BitNextPow2(1) == 1);
    assert(BitNextPow2(2) == 2);
    assert(BitNextPow2(5) == 8);
    assert(BitNextPow2(1000) == 1024);

    /* Dot notation */
    assert(bit.popcount32(0x0F) == 4);
    assert(bit.isPow2(64));
    assert(!bit.isPow2(65));
}

static void test_base64(void) {
    /* Test vectors from RFC 4648 */
    char enc[128];
    u8 dec[128];
    CP_UNUSED(enc); CP_UNUSED(dec);

    assert(Base64Encode("", 0, enc, sizeof(enc)) == 0);
    assert(strcmp(enc, "") == 0);

    assert(Base64Encode("f", 1, enc, sizeof(enc)) == 4);
    assert(strcmp(enc, "Zg==") == 0);

    assert(Base64Encode("fo", 2, enc, sizeof(enc)) == 4);
    assert(strcmp(enc, "Zm8=") == 0);

    assert(Base64Encode("foo", 3, enc, sizeof(enc)) == 4);
    assert(strcmp(enc, "Zm9v") == 0);

    assert(Base64Encode("foob", 4, enc, sizeof(enc)) == 8);
    assert(strcmp(enc, "Zm9vYg==") == 0);

    assert(Base64Encode("fooba", 5, enc, sizeof(enc)) == 8);
    assert(strcmp(enc, "Zm9vYmE=") == 0);

    assert(Base64Encode("foobar", 6, enc, sizeof(enc)) == 8);
    assert(strcmp(enc, "Zm9vYmFy") == 0);

    /* Decode RFC 4648 vectors */
    i64 dlen = Base64Decode("Zm9vYmFy", 8, dec, sizeof(dec));
    assert(dlen == 6);
    dec[dlen] = '\0';
    assert(strcmp((char *)dec, "foobar") == 0);

    /* Binary roundtrip with 256 bytes */
    u8 raw[256];
    for (usize i = 0; i < 256; i++) raw[i] = (u8)i;

    char enc_bin[512];
    i64 enc_len = Base64Encode(raw, 256, enc_bin, sizeof(enc_bin));
    assert(enc_len > 0);

    u8 dec_bin[512];
    i64 dec_bin_len = Base64Decode(enc_bin, (usize)enc_len, dec_bin, sizeof(dec_bin));
    assert(dec_bin_len == 256);
    assert(memcmp(raw, dec_bin, 256) == 0);
    CP_UNUSED(dec_bin_len);

    /* Arena versions */
    cp_arena_t *arena = ArenaCreate(64 * 1024);
    cp_strview_t sv_enc = Base64EncodeArena("Hello, Cplus World!", 19, arena);
    assert(sv_enc.len > 0);

    usize out_len = 0;
    void *decoded = Base64DecodeArena(sv_enc.data, sv_enc.len, arena, &out_len);
    assert(out_len == 19);
    assert(memcmp(decoded, "Hello, Cplus World!", 19) == 0);
    CP_UNUSED(decoded);

    ArenaDestroy(arena);
}

static void test_prng_and_logger(void) {
    cp_rand_t rng_local;
    RandInit(&rng_local, 987654321ULL);

    u64 v1 = RandU64(&rng_local);
    u64 v2 = RandU64(&rng_local);
    assert(v1 != v2);
    CP_UNUSED(v1); CP_UNUSED(v2);

    f64 f = RandF64(&rng_local);
    assert(f >= 0.0 && f < 1.0);
    CP_UNUSED(f);

    for (int i = 0; i < 1000; i++) {
        u64 r = RandRange(&rng_local, 50, 100);
        assert(r >= 50 && r <= 100);
        CP_UNUSED(r);
    }

    /* Global PRNG */
    RandSeed(11223344);
    u64 g1 = RandNextU64();
    u64 g2 = RandNextU64();
    assert(g1 != g2);
    CP_UNUSED(g1); CP_UNUSED(g2);

    /* Dot-notation */
    u64 dr = rng.range(&rng_local, 1, 5);
    assert(dr >= 1 && dr <= 5);
    CP_UNUSED(dr);

    /* Logger */
    LogSetLevel(CP_LOG_LEVEL_INFO);
    LogDebug("This debug message will not show");
    LogInfo("Testing Cplus high-performance logger: %s (version %d.%d.%d)", "OK", 0, 1, 0);
    LogWarn("Warning test message: %d%% battery", 15);
    LogError("Error test message (simulated): %s", "File not found");
}

static void test_lambda_and_type_aliases(void) {
#if CP_HAS_LAMBDA
    /* Test Lambda macro */
    int (*add_three)(int) = Lambda(int, (int x), { return x + 3; });
    assert(add_three(7) == 10);
    CP_UNUSED(add_three);

    int arr[] = { 42, 12, 88, 5, 19 };
    AlgoSort(arr, 5, sizeof(int), Lambda(int, (const void *a, const void *b), {
        return (*(const int *)a - *(const int *)b);
    }));
    assert(arr[0] == 5 && arr[1] == 12 && arr[2] == 19 && arr[3] == 42 && arr[4] == 88);
#endif

    /* Test Clean Type Aliases */
    Arena *arena = ArenaCreate(2048);
    assert(arena != NULL);

    HashMap *map = MapCreate(16, arena);
    assert(map != NULL);
    MapSet(map, "port", (void*)(uintptr_t)8080);
    assert((uintptr_t)MapGet(map, "port") == 8080);

    Stack *stk = StackCreate(8, arena);
    assert(stk != NULL);
    StackPush(stk, (void*)(uintptr_t)101);
    assert((uintptr_t)StackPop(stk) == 101);

    String str = StrNew(arena, 32);
    StrAppendC(&str, "C+ Clean Types");
    assert(StrLen(&str) == 14);

    Rng rng_var;
    RandInit(&rng_var, 42);
    assert(RandU64(&rng_var) != 0);

    RingBuffer *rb = RingBufCreate(8);
    assert(rb != NULL);
    RingBufDestroy(rb);

    ArenaDestroy(arena);
}

int main(void) {
    printf("Running test_utils...\n");
    test_bit_intrinsics();
    test_base64();
    test_prng_and_logger();
    test_lambda_and_type_aliases();
    printf("All test_utils unit tests passed successfully!\n");
    return 0;
}
