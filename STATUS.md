Directive 003 — status (2026-10-30)

Section 1 baseline: preserved under native/c/baseline/ with hashes and
transcript. Baseline driver run (seed 20261031): 500 ops, 318 loads,
182 stores, hits=309, misses=9, byte-identical on rerun. Census of the
baseline's load values against the driver's own dumped image: 290 of
318 values wrong (line staleness from the line-granular slot semantics
and the zero-initial incoherence of the supplied harness).

Section 2 bounds: implemented in native/c/memcache.c. range_valid is
checked before any address arithmetic; the width contract is
{1,2,4,8}; invalid loads return 0 and invalid stores write nothing;
both increment the errors counter. Slot semantics hold the aligned
64-bit word of the access address with byte extraction; word-crossing
accesses are served exactly from backing; stores refresh the slot of
every word they touch.

Section 3 + section 5 (C side): native/c/memcache_bounds_test.c reports
1583 checks, 0 failures, under ASan+UBSan. The section-1 OOB case
(capacity 64, load addr 60 width 8) now returns 0 as a rejection;
errors=1.

Section 4 differential: run2/ artifacts. All five checks pass:
  A  PRNG stream: Wolfram-regenerated 500 trace ops identical (500/500)
  B  initial memory: Wolfram-regenerated image identical (1024/1024)
  C  load values: Wolfram replay vs C replay agree (318/318)
  D  final memory: byte-identical (1024/1024)
  E  validity grid: C range_valid vs the Wolfram invariant over
     9,909 (addr, width, capacity) rows, 0 disagreements
The differential found one defect in the fixed cache (a store crossing
a word boundary left the second word's slot stale: 7 of 318 loads);
the fix and its regression case are in the bounds suite.
Wolfram-side note: in this environment Mathics3 evaluates no Bit*
builtins and cannot import JSON, so the runners express the same
recurrences with Quotient/Mod/IntegerDigits primitives and a
hand-rolled trace reader. XorShift32 under those primitives maps
seed 1 to 270369 as expected.

Section 6 provenance: verification/PROVENANCE.md (SovereignL0 SKI
proof, Coq transcript, execution harness hashes).

Section 7 gate: OOB rejected (yes); native and Wolfram semantics agree
(yes, checks A-E); every mismatch resolved or documented (yes: baseline
census above, crossing-store defect fixed, initial-state incoherence
documented in the baseline transcript); sanitizer results clean (yes);
evidence reproducible from this tree (yes).
