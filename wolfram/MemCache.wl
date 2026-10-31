(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* ---------- wolfram/MemCache.wl ---------- *)
ClearAll[CacheLoad, CacheStore, CacheReference];

CacheReference[mem_List, addr_, width_, slots_ : 4096, lineShift_ : 6] :=
  Module[
    {tag = BitShiftRight[addr, lineShift],
     slot = BitAnd[tag, slots - 1],
     word},
    (* miss path only: Wolfram has no persistent state here, so the
       reference computes the raw load; hit/miss accounting is driven
       by the C side's tag array. See differential harness below. *)
    word = Fold[
      BitOr[#2, BitShiftLeft[#1, 8]] &,
      Reverse[Take[mem, {addr + 1, addr + width}]]  (* little-endian *)
    ];
    <|"Tag" -> tag, "Slot" -> slot, "Value" -> word|>
  ];

(* Differential oracle: replay a C-side trace (addr, width, op, val)
   and recompute expected values from a pure Wolfram memory image. *)
ReplayTrace[mem_List, trace_List] :=
  Module[{m = mem, out = {}},
    Do[
      Which[
        t["Op"] == "Load",
        AppendTo[out, <|"Addr" -> t["Addr"], "Width" -> t["Width"],
          "Expected" -> FromLittleEndian[
            Take[m, {t["Addr"] + 1, t["Addr"] + t["Width"]}]]|>],
        t["Op"] == "Store",
        Do[m[[t["Addr"] + i]] =
             BitAnd[BitShiftRight[t["Val"], 8 (i - 1)], 255],
          {i, t["Width"]}]],
      {t, trace}];
    out
  ];

Needs["http`Client`"]; (* or shell out to the compiled binary *)

DifferentialCacheTest[] := Module[
  {mem, trace, expected, actualSeed = 20261031},
  SeedRandom[actualSeed];
  mem = RandomInteger[255, 1024];
  trace = Table[
    With[{a = RandomInteger[0, 900], w = RandomChoice[{1, 2, 4, 8}]},
      <|"Op" -> "Load", "Addr" -> a, "Width" -> w|>],
    {200}];
  expected = ReplayTrace[mem, trace];
  (* actual = run native/c/memcache_test with identical seed and mem dump;
     compare field-wise. *)
  <|"Expected" -> expected, "Native" -> "NOT EXECUTED — binary not yet built"|>
]
