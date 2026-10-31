/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
/* Replay tool (Directive 003 section 4): loads mem.bin into the cache
 * through valid stores, replays trace.jsonl, writes load results in the
 * driver's format, and exports the final logical memory image through a
 * complete set of valid width-1 reads (final.bin).
 * Usage: ./memcache_replay <mem.bin> <trace.jsonl> <results.jsonl> <final.bin>
 */
#include "memcache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 5) { fprintf(stderr,
        "usage: %s mem.bin trace.jsonl results.jsonl final.bin\n", argv[0]);
        return 2; }

    const uint32_t MEM = 1024;
    uint8_t *image = malloc(MEM);
    FILE *fm = fopen(argv[1], "rb");
    if (fread(image, 1, MEM, fm) != MEM) { fprintf(stderr, "short mem\n"); return 1; }
    fclose(fm);

    Cache *c = cache_create(MEM);
    for (uint32_t a = 0; a < MEM; a += 8) {          /* initial image */
        uint64_t w = 0;
        for (uint32_t i = 0; i < 8; i++) w |= ((uint64_t)image[a + i]) << (8u * i);
        cache_store(c, a, 8, w);
    }

    FILE *ft = fopen(argv[2], "r");
    FILE *fr = fopen(argv[3], "w");                  /* results.jsonl */
    char line[512];
    unsigned long ops = 0, loads = 0;
    while (fgets(line, sizeof line, ft)) {
        unsigned a, w; unsigned long long val;
        if (strstr(line, "\"Load\"")) {
            if (sscanf(line, "{\"Op\":\"Load\",\"Addr\":%u,\"Width\":%u}", &a, &w) != 2)
                { fprintf(stderr, "bad load line: %s", line); return 1; }
            uint64_t v = cache_load(c, a, w);
            fprintf(fr, "{\"Addr\":%u,\"Width\":%u,\"Value\":%llu}\n",
                    a, w, (unsigned long long)v);
            loads++;
        } else if (strstr(line, "\"Store\"")) {
            if (sscanf(line, "{\"Op\":\"Store\",\"Addr\":%u,\"Width\":%u,\"Val\":%llu}",
                       &a, &w, &val) != 3)
                { fprintf(stderr, "bad store line: %s", line); return 1; }
            cache_store(c, a, w, val);
        } else if (line[0] != '\n' && line[0] != '\0') {
            fprintf(stderr, "unknown line: %s", line); return 1;
        }
        ops++;
    }
    fclose(ft); fclose(fr);

    FILE *ff = fopen(argv[4], "wb");                 /* final image */
    for (uint32_t a = 0; a < MEM; a++)
        fputc((int)cache_load(c, a, 1), ff);
    fclose(ff);

    fprintf(stderr, "replay: ops=%lu loads=%lu hits=%llu misses=%llu errors=%llu\n",
            ops, loads, (unsigned long long)c->hits,
            (unsigned long long)c->misses, (unsigned long long)c->errors);
    cache_destroy(c); free(image);
    return 0;
}
