# memcache — flat, single-allocation, pointer-arithmetic cache

**Owners: Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty.**
**Our trademark is our watermarks and holographic images we drop on every repo.**

A byte-addressable memory cache in C11 with a Wolfram Language reference
model and a differential test harness. One malloc backs the whole cache:
backing store, word slots, and tags are carved out of a single allocation
by pointer arithmetic.

## Model

- Width-tagged accesses of 1, 2, 4, or 8 bytes, little-endian.
- Direct-mapped slots, 4096 of them. A slot holds the aligned 64-bit word
  containing the address (`tag = addr >> 3`, `slot = tag & (slots - 1)`);
  a load extracts the requested bytes from that word, including accesses
  that cross a word boundary.
- Stores are write-through and write-allocate.

## Access contract

An access is valid if and only if the width is in {1, 2, 4, 8} and
`addr <= capacity` and `width <= capacity - addr` (overflow-safe form).
An invalid load returns 0 and an invalid store writes nothing; both
increment the cache's `errors` counter, which is the independent status
channel that distinguishes a rejected load from a load whose valid
value is 0.

The same predicate is the formal invariant of the Wolfram model,
`Valid(a, w, M) ⟺ 0 ≤ a ≤ M ∧ 0 < w ≤ M − a`, and the two are tested for
equivalence over a dumped grid (9,909 rows).

## Layout

- `native/c/` — the cache (`memcache.c`), the deterministic driver
  (`memcache_test.c`, xorshift32, seed-driven), the replay/export tool
  (`memcache_replay.c`), the boundary and contract tests
  (`memcache_bounds_test.c`, `memcache_oob.c`), and the range-grid
  dumper (`range_grid.c`).
- `native/c/baseline/` — the pre-contract execution evidence: source
  and binary SHA-256 hashes, the seed-20261031 transcript
  (500 ops, 318 loads, 182 stores), and the captured artifacts.
- `native/c/run2/` — the current execution artifacts: memory image,
  trace, driver and replay results, the exported final memory image,
  and the range grid.
- `wolfram/` — the reference model (`MemCache.wl`), the differential
  runner (`CacheDifferential.wl`, `CacheDiffAB.wl`, `CacheDiffCD.wl`,
  `CacheDiffE.wl`), and the supplied verification tests
  (`MemCacheTests.wlt`). The Wolfram side regenerates the full PRNG
  stream, replays all 500 operations, compares all 318 load values by
  sequence number, and compares the final memory image byte for byte.

## Build and verify

```sh
cd native/c
make                                    # clang, ASan + UBSan
./memcache_test 20261031 run2/mem.bin run2/trace.jsonl run2/results.jsonl

clang -O1 -g -Wall -Wextra -fsanitize=address,undefined \
  -fno-sanitize-recover=all -o memcache_bounds_test memcache_bounds_test.c
./memcache_bounds_test                  # 1580 checks

clang -O1 -g -Wall -Wextra -fsanitize=address,undefined \
  -fno-sanitize-recover=all -o memcache_replay memcache.c memcache_replay.c
./memcache_replay run2/mem.bin run2/trace.jsonl \
  run2/results_replay.jsonl run2/final.bin
```

## License

See `LICENSE`. Commercial exploitation requires separate written
authorization from the owners; publication, access, download, or
possession is not that authorization.
