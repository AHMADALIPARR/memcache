/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
/* memcache.h — harness header for the memcache differential build.
 * The Cache struct and prototypes are the declarations of
 * native/c/memcache.c (which ships no header of its own); the struct
 * text is copied unchanged so memcache_test.c can read hits/misses.
 * The byte_at/word_at accessor pair below is the owner's, verbatim
 * as supplied 2026-10-30. */
#ifndef MEMCACHE_H
#define MEMCACHE_H

#include <stdint.h>

typedef struct {
    uint8_t  *mem;                      /* backing store, one malloc */
    uint64_t *words;                     /* cache: mem + wordRegion   */
    uint32_t *tags;                      /* cache: mem + tagRegion    */
    uint32_t  memBytes;
    uint64_t  hits;
    uint64_t  misses;
    uint64_t  errors;                    /* rejected accesses         */
} Cache;

Cache   *cache_create(uint32_t memBytes);
uint64_t cache_load(Cache *c, uint32_t addr, uint32_t width);
void     cache_store(Cache *c, uint32_t addr, uint32_t width, uint64_t val);
void     cache_destroy(Cache *c);

uint8_t  *byte_at (Cache *c, uint32_t addr) { return c->mem + addr; }
uint64_t *word_at (Cache *c, uint32_t idx)  { return c->words + idx; }

#endif
