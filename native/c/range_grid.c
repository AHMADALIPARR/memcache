/* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty */
#include "memcache.c"
#include <stdio.h>
int main(void) {
    static const size_t ms[] = {0, 1, 64, 1024};
    static const size_t ws[] = {0, 1, 2, 3, 4, 8, 16, 64, 1025};
    for (unsigned mi = 0; mi < 4; mi++) {
        size_t m = ms[mi];
        for (size_t a = 0; a <= m + 2; a++)
            for (unsigned wi = 0; wi < 9; wi++)
                printf("%zu,%zu,%zu,%d\n", a, ws[wi], m, range_valid(a, ws[wi], m));
    }
    return 0;
}
