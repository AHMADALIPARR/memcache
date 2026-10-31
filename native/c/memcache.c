/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
/* ==========================================================================
 * native/c/memcache.c — flat, single-allocation, pointer-arithmetic cache
 * Model: byte-addressable store of width-tagged words (1/2/4/8 bytes)
 * Policy: direct-mapped, 64-bit word slots
 *   => direct-mapped: tag = addr >> 3, slot = tag & (slots-1);
 *      a slot holds the aligned 64-bit word containing the address,
 *      and a load extracts the requested bytes from that word.
 * Access contract: an access is valid iff width ∈ {1,2,4,8} and
 *   addr <= memBytes and width <= memBytes - addr.
 *   An invalid load returns 0 and an invalid store writes nothing;
 *   both increment the errors counter, which is the independent
 *   status channel that distinguishes a rejected load from a load
 *   whose valid value is 0.
 * ========================================================================== */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CACHE_SLOTS   (1u << 12)        /* 4096 slots, power of two */
#define WORD_STRIDE    8u                /* bytes per word slot      */
#define WORD_SHIFT     3u                /* 64-bit words             */

typedef struct {
    uint8_t  *mem;                      /* backing store, one malloc */
    uint64_t *words;                     /* cache: mem + wordRegion   */
    uint32_t *tags;                      /* cache: mem + tagRegion    */
    uint32_t  memBytes;
    uint64_t  hits;
    uint64_t  misses;
    uint64_t  errors;                    /* rejected accesses         */
} Cache;

/* ---- the pointer-arithmetic kernels ---- */
static inline uint64_t *word_at(Cache *c, uint32_t slot) {
    return c->words + slot;              /* base + slot * 8 via scaling */
}
static inline uint32_t *tag_at(Cache *c, uint32_t slot) {
    return c->tags + slot;               /* base + slot * 4            */
}

/* ---- logical bounds: overflow-safe range validation ---- */
static inline int range_valid(
    size_t address,
    size_t width,
    size_t capacity
) {
    return address <= capacity &&
           width <= capacity - address;
}

/* the documented width contract: width-tagged words of 1/2/4/8 bytes */
static inline int width_valid(uint32_t width) {
    return width == 1u || width == 2u || width == 4u || width == 8u;
}

static inline int access_valid(Cache *c, uint32_t addr, uint32_t width) {
    return width_valid(width) &&
           range_valid(addr, width, c->memBytes);
}

/* Cache allocation: ONE malloc, regions carved by pointer arithmetic.
 * Layout: [ backing store | words (8B * SLOTS) | tags (4B * SLOTS) ]
 * Alignment: backing store size rounded up to 8. */
Cache *cache_create(uint32_t memBytes) {
    Cache *c = malloc(sizeof *c);
    uint32_t storeBytes = (memBytes + 7u) & ~7u;
    size_t   total = (size_t)storeBytes
                   + (size_t)CACHE_SLOTS * WORD_STRIDE
                   + (size_t)CACHE_SLOTS * 4u;
    c->mem = malloc(total);
    c->memBytes  = memBytes;
    c->words     = (uint64_t *)(c->mem + storeBytes);   /* + arithmetic */
    c->tags      = (uint32_t *)(c->mem + storeBytes
                                 + (size_t)CACHE_SLOTS * WORD_STRIDE);
    c->hits = c->misses = c->errors = 0;
    memset(c->mem, 0, total);
    return c;
}

/* the aligned 64-bit word of the backing store that contains addr */
static inline uint64_t backing_word(Cache *c, uint32_t addr) {
    uint64_t w = 0;
    uint8_t *p = c->mem + (addr & ~7u);           /* base + offset */
    for (uint32_t i = 0; i < 8u; i++) {
        uint32_t byte = (uint32_t)(*p++) & 0xFFu;
        w |= ((uint64_t)byte) << (8u * i);        /* little-endian */
    }
    return w;
}

/* ---- load: width-tagged, little-endian, offset arithmetic only ---- */
uint64_t cache_load(Cache *c, uint32_t addr, uint32_t width) {
    if (!access_valid(c, addr, width)) {          /* check before use */
        c->errors++;
        return 0;
    }

    uint32_t tag  = addr >> WORD_SHIFT;
    uint32_t slot = tag & (CACHE_SLOTS - 1);      /* direct map */
    uint32_t off  = addr & 7u;

    if (off + width > 8u) {
        /* the access crosses a word boundary: serve it from the
           backing store; the value is exact and nothing is cached */
        c->misses++;
        uint64_t w = 0;
        uint8_t *p = c->mem + addr;
        for (uint32_t i = 0; i < width; i++) {
            uint32_t byte = (uint32_t)(*p++) & 0xFFu;
            w |= ((uint64_t)byte) << (8u * i);
        }
        return w;
    }

    if (*tag_at(c, slot) != tag + 1u) {           /* tag 0 = invalid */
        c->misses++;
        *word_at(c, slot) = backing_word(c, addr);
        *tag_at(c, slot) = tag + 1u;
    } else {
        c->hits++;
    }
    uint64_t w = *word_at(c, slot);
    return (w >> (8u * off)) & ((width == 8u) ? ~0ULL : ((1ULL << (8u * width)) - 1ULL));
}

/* ---- store: write-through, updates both cache and backing store ---- */
void cache_store(Cache *c, uint32_t addr, uint32_t width, uint64_t val) {
    if (!access_valid(c, addr, width)) {          /* check before use */
        c->errors++;
        return;
    }

    /* backing store: byte-wise pointer arithmetic again */
    uint8_t *p = c->mem + addr;
    for (uint32_t i = 0; i < width; i++) {
        *p++ = (uint8_t)(val >> (8u * i));
    }
    /* write-through, write-allocate: the slot holds the aligned word */
    uint32_t tag  = addr >> WORD_SHIFT;
    uint32_t slot = tag & (CACHE_SLOTS - 1);
    *word_at(c, slot) = backing_word(c, addr);
    *tag_at(c, slot) = tag + 1u;
}

void cache_destroy(Cache *c) { free(c->mem); free(c); }
