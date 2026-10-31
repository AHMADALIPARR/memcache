/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
/* Deterministic cache driver: emits trace to stdout, results to file.
 * Usage: ./memcache_test <seed> <memdump.bin> <trace.jsonl> <results.jsonl>
 */
#include "memcache.h"
#include <stdio.h>
#include <stdlib.h>

/* xorshift32 — fixed, specified, reproducible in Wolfram */
static uint32_t xs_state;
static uint32_t xs32(void) {
    uint32_t x = xs_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return xs_state = x;
}

int main(int argc, char **argv) {
    if (argc != 5) { fprintf(stderr,
        "usage: %s seed memdump.bin trace.jsonl results.jsonl\n", argv[0]);
        return 2; }

    uint32_t seed = (uint32_t)strtoul(argv[1], NULL, 10);
    xs_state = seed ? seed : 1;                 /* 0 is illegal for xorshift */

    const uint32_t MEM = 1024;
    uint8_t *image = malloc(MEM);
    for (uint32_t i = 0; i < MEM; i++) image[i] = (uint8_t)xs32();

    Cache *c = cache_create(MEM);

    FILE *fm = fopen(argv[2], "wb");
    fwrite(image, 1, MEM, fm); fclose(fm);      /* Wolfram reads this */

    FILE *ft = fopen(argv[3], "w");
    FILE *fr = fopen(argv[4], "w");

    for (int t = 0; t < 500; t++) {
        uint32_t a  = xs32() % (MEM - 8);       /* keep addr+width in bounds */
        uint32_t w  = (uint32_t)1u << (xs32() % 3);   /* width ∈ {1,2,4} */
        int      op = (int)(xs32() % 3);              /* 0,1 = load; 2 = store */

        if (op < 2) {
            uint64_t v = cache_load(c, a, w);
            fprintf(ft, "{\"Op\":\"Load\",\"Addr\":%u,\"Width\":%u}\n", a, w);
            fprintf(fr, "{\"Addr\":%u,\"Width\":%u,\"Value\":%llu}\n",
                    a, w, (unsigned long long)v);
        } else {
            uint64_t v = ((uint64_t)xs32() << 32) | xs32();
            cache_store(c, a, w, v & ((1ULL << (8*w)) - 1));
            fprintf(ft,
              "{\"Op\":\"Store\",\"Addr\":%u,\"Width\":%u,\"Val\":%llu}\n",
              a, w, (unsigned long long)(v & ((1ULL << (8*w)) - 1)));
        }
    }

    /* hit/miss accounting — asserted, not claimed */
    fprintf(stderr, "hits=%llu misses=%llu\n",
            (unsigned long long)c->hits, (unsigned long long)c->misses);

    cache_destroy(c); free(image);
    fclose(ft); fclose(fr);
    return 0;
}
