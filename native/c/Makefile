# Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty
CC      = clang
SAN     = -fsanitize=address,undefined -fno-sanitize-recover=all
CFLAGS  = -O1 -g -Wall -Wextra $(SAN)

all: memcache_test

memcache_test: memcache.c memcache.h memcache_test.c
	$(CC) $(CFLAGS) -o $@ memcache.c memcache_test.c

clean:
	rm -f memcache_test *.bin *.jsonl

.PHONY: all clean
