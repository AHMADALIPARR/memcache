(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* Replays trace.jsonl against mem.bin and compares to results.jsonl *)
ClearAll[XorShift32, RunCacheDifferential];

(* Byte-exact reimplementation of the C PRNG — first thing tested.
   Unsigned 32-bit arithmetic is enforced by masking after each
   shift/XOR stage (Directive 003 section 4); the unmasked original
   is preserved in baseline/CacheDifferential.wl.orig. *)
XorShift32[x_] := Module[{y = BitAnd[BitXor[x, BitShiftLeft[x, 13]], 2^32 - 1]},
  y = BitAnd[BitXor[y, BitShiftRight[y, 17]], 2^32 - 1];
  BitAnd[BitXor[y, BitShiftLeft[y, 5]], 2^32 - 1]];

RunCacheDifferential[dir_String] := Module[
  {image, traceLines, resultLines, trace, results, m, got, mismatches, i, t, ok, checks},

  image = Import[FileNameJoin[{dir, "mem.bin"}], "Byte"];
  If[Length[image] =!= 1024, Return[<|"Error" -> "mem.bin size ≠ 1024"|>]];

  (* ---- Check A: PRNG equivalence (xorshift32) ---- *)
  Module[{s = 20261031, expected = Table[0, 3], ok = True},
    Do[expected[[k + 1]] = (s = XorShift32[s]; BitAnd[s, 255]), {k, 0, 2}];
    checks = {<|"Check" -> "PRNG-Head", "ExpectedFirst3" -> expected,
                "ImageHead3" -> Take[image, 3],
                "Match" -> expected === Take[image, 3]|>}];

  traceLines = StringSplit[Import[FileNameJoin[{dir, "trace.jsonl"}], "Text"], "\n"];
  resultLines = Select[StringSplit[
      Import[FileNameJoin[{dir, "results.jsonl"}], "Text"], "\n"], # != "" &];
  trace = ImportString[StringRiffle[traceLines, ","], "JSON"]; (* array form *)

  m = image; mismatches = {}; got = {};
  Do[
    Which[
      t["Op"] === "Load",
      AppendTo[got,
        FromDigits[Reverse[Take[m, {t["Addr"] + 1, t["Addr"] + t["Width"]}]],
          256]],
      t["Op"] === "Store",
      Do[m[[t["Addr"] + i]] =
           BitAnd[BitShiftRight[t["Val"], 8 (i - 1)], 255],
        {i, t["Width"]}]],
    ] /. Null -> Null,
    {t, trace}];

  (* ---- Check B: value equivalence over all 500 ops ---- *)
  Module[{expectedVals},
    expectedVals = Map[
      (ImportString[#, "JSON"]["Value"]) &, resultLines];
    mismatches = Select[
      MapIndexed[
        If[#1 =!= expectedVals[[#2[[1]]]],
          <|"Index" -> #2[[1]], "Got" -> #1,
            "Expected" -> expectedVals[[#2[[1]]]]|>, Nothing] &,
        got];
    checks = Join[checks, {<|
      "Check" -> "ValueEquivalence",
      "OpsCompared" -> Length[got],
      "Mismatches" -> Length[mismatches],
      "Verdict" -> If[mismatches === {}, "PASS", "FAIL"]|>}]
  ];

  <|"Checks" -> checks, "MismatchDetail" -> mismatches|>
]
