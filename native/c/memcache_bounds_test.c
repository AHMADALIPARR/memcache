/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
/* Boundary regression + exhaustive access-contract tests (Directive 003
 * sections 3 and 5, C side). Includes memcache.c to reach range_valid.
 * Prints one line per failure and a summary; exit code = failures != 0. */
#include "memcache.c"
#include <stdio.h>
#include <string.h>
#include <limits.h>

static int checks = 0, failures = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; \
    printf("FAIL %s:%d: ", __func__, __LINE__); printf(__VA_ARGS__); \
    printf("\n"); } } while (0)

static uint64_t backing_bytes(Cache *c, uint32_t addr, uint32_t width) {
    uint64_t w = 0;
    for (uint32_t i = 0; i < width; i++)
        w |= ((uint64_t)c->mem[addr + i]) << (8u * i);
    return w;
}

static void boundary_cases(void) {
    Cache *c = cache_create(64);
    for (uint32_t i = 0; i < 64; i++) c->mem[i] = (uint8_t)(i + 1);
    uint64_t e;

    e = c->errors; (void)cache_load(c, 60, 4);
    CHECK(c->errors == e, "cap64 addr60 w4 load must be valid");
    e = c->errors; CHECK(cache_load(c, 60, 8) == 0 && c->errors == e + 1,
        "cap64 addr60 w8 load must be rejected");
    e = c->errors; cache_store(c, 60, 8, 0xFF);
    CHECK(c->errors == e + 1 && backing_bytes(c, 60, 4) == 0x403F3E3DULL,
        "cap64 addr60 w8 store must be rejected and write nothing");
    e = c->errors; (void)cache_load(c, 63, 1);
    CHECK(c->errors == e, "cap64 addr63 w1 load must be valid");
    e = c->errors; CHECK(cache_load(c, 64, 1) == 0 && c->errors == e + 1,
        "cap64 addr64 w1 load must be rejected");
    e = c->errors; cache_store(c, 64, 1, 0xAA);
    CHECK(c->errors == e + 1, "cap64 addr64 w1 store must be rejected");
    e = c->errors; CHECK(cache_load(c, 0, 64) == 0 && c->errors == e + 1,
        "cap64 addr0 w64: outside the {1,2,4,8} width contract, rejected");
    e = c->errors; CHECK(cache_load(c, 0, 0) == 0 && c->errors == e + 1,
        "cap64 addr0 w0: violates 0 < w, rejected");
    cache_destroy(c);
}

static void range_valid_edges(void) {
    CHECK(range_valid(60, 4, 64) == 1, "60+4<=64 valid");
    CHECK(range_valid(60, 8, 64) == 0, "60+8>64 invalid");
    CHECK(range_valid(64, 1, 64) == 0, "64+1>64 invalid");
    CHECK(range_valid(0, SIZE_MAX, SIZE_MAX) == 1, "full range at SIZE_MAX");
    CHECK(range_valid(SIZE_MAX, 1, SIZE_MAX) == 0, "no room at SIZE_MAX");
    CHECK(range_valid(SIZE_MAX - 3, 4, SIZE_MAX) == 0, "overflow-safe: 4 > 3 left");
    CHECK(range_valid(SIZE_MAX - 3, 3, SIZE_MAX) == 1, "exactly fits at SIZE_MAX");
    CHECK(range_valid(SIZE_MAX, 0, SIZE_MAX) == 1,
        "range_valid alone admits width 0; the cache contract rejects it via width_valid");
}

static void value_semantics(void) {
    Cache *c = cache_create(64);
    cache_store(c, 0, 4, 0xAABBCCDDULL);
    cache_store(c, 8, 2, 0x1234ULL);
    CHECK(cache_load(c, 0, 4) == 0xAABBCCDDULL, "same-line addr 0 keeps its value");
    CHECK(cache_load(c, 8, 2) == 0x1234ULL, "same-line addr 8 keeps its value");
    CHECK(cache_load(c, 1, 2) == 0xBBCCULL, "unaligned sub-word extraction");
    cache_store(c, 4, 4, 0x11223344ULL);
    cache_store(c, 8, 4, 0x55667788ULL);
    CHECK(cache_load(c, 6, 4) == 0x77881122ULL, "word-crossing load is exact");
    CHECK(c->errors == 0, "no rejections in value semantics");
    cache_destroy(c);
}

static void exhaustive_contract(void) {
    static const uint32_t widths[] = {0, 1, 2, 3, 4, 5, 8, 16, 64};
    for (uint32_t a = 0; a <= 72; a++) {
        for (unsigned wi = 0; wi < sizeof(widths) / sizeof(widths[0]); wi++) {
            uint32_t w = widths[wi];
            /* fresh cache per cell: the initial image is poked into the
               backing store before any access, while all tags are invalid */
            Cache *c = cache_create(64);
            for (uint32_t i = 0; i < 64; i++) c->mem[i] = (uint8_t)(0x80 + i);
            int valid = (w == 1 || w == 2 || w == 4 || w == 8) &&
                        range_valid(a, w, 64);
            uint64_t e = c->errors;
            if (valid) {
                uint64_t want = backing_bytes(c, a, w);
                uint64_t got = cache_load(c, a, w);
                CHECK(got == want && c->errors == e,
                    "load a=%u w=%u valid: got %llu want %llu",
                    a, w, (unsigned long long)got, (unsigned long long)want);
                cache_store(c, a, w, 0xDEADBEEFCAFEULL);
                CHECK(c->errors == e &&
                      backing_bytes(c, a, w) == (0xDEADBEEFCAFEULL & ((w == 8) ? ~0ULL : ((1ULL << (8 * w)) - 1))),
                    "store a=%u w=%u valid: backing updated", a, w);
                CHECK(cache_load(c, a, w) ==
                      (0xDEADBEEFCAFEULL & ((w == 8) ? ~0ULL : ((1ULL << (8 * w)) - 1))),
                    "load a=%u w=%u after store: cached value agrees", a, w);
            } else {
                uint8_t before[64]; memcpy(before, c->mem, 64);
                uint64_t got = cache_load(c, a, w);
                CHECK(got == 0 && c->errors == e + 1,
                    "load a=%u w=%u invalid: rejected", a, w);
                cache_store(c, a, w, 0xDEADBEEFCAFEULL);
                CHECK(c->errors == e + 2 && memcmp(before, c->mem, 64) == 0,
                    "store a=%u w=%u invalid: rejected, backing untouched", a, w);
            }
            cache_destroy(c);
        }
    }
}

int main(void) {
    boundary_cases();
    range_valid_edges();
    value_semantics();
    exhaustive_contract();
    printf("BOUNDS-TEST: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
