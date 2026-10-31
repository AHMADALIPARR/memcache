/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
/* Confirms the declared OOB defect is DETECTABLE by ASan.
 * Expected: ASan aborts. If it does not abort, either the sanitizer
 * is broken or the defect was already fixed — update the ledger. */
#include "memcache.h"
#include <stdio.h>
int main(void) {
    Cache *c = cache_create(64);
    printf("%llu\n", (unsigned long long)cache_load(c, 60, 8)); /* 60+8 > 64 */
    return 0;
}
