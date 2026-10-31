# Baseline evidence — GHOST Directive 003 §1 (captured 2026-10-30, before any fix)

## Identity
- The ghost tree is not a git repository; the source identity anchor is SHA-256.
- memcache.c        sha256 90680de6920d28c9501a4e60a5584a0ff3936e0ad6c8912e9152abf61f2b3dd2
- memcache_test     sha256 106370d71c44821e6a5c72e15653930220e6ce399ec53dce7d443a4821fdc61e (baseline binary)
- memcache_oob      sha256 2c79a9ffcc6bf4de3c5b2c9d7765c550f366b1cc554d0d10f04fbe1603d0d40d (baseline binary)
- Compiler: clang, flags from the supplied Makefile:
  -O1 -g -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all

## Failing-baseline results (seed 20261031)
- Operations: 500 (loads 318, stores 182)
- Hits: 309, misses: 9 (stderr transcript: driver_stderr.txt)
- Artifacts: mem.bin, trace.jsonl, results.jsonl (byte-identical across a
  rerun — determinism verified by cmp, see the check_* copies)
- Out-of-bounds test (memcache_oob.c): capacity 64, load offset 60 width 8
  - stdout: 0, exit code 0 (oob_stdout.txt)
  - ASan result: no reported violation
  - Logical bounds result: VIOLATION — the access reads past the declared
    64-byte capacity; it stays inside the single backing allocation
    (words region), so ASan cannot see it.
- Value semantics (established by the Wolfram replay design): on a tag hit
  the baseline returns the cached word of whichever access last touched
  the 64-byte line, not the bytes at the requested address — hits are not
  value-correct except when address and width coincide.

Source copies in this directory are byte-identical to the originals at
capture time (memcache.c, memcache.h, memcache_test.c, memcache_oob.c,
Makefile).
