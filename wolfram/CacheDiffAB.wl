(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* CacheDiffAB.wl -- Directive 003 section 4, checks A and B:
   full PRNG stream equivalence + initial memory equality.
   JSONL parsed with Wolfram string patterns (Mathics3's JSON importer
   fails on these records; RegularExpression rule-delayed forms are
   avoided for the same reason). *)
XorShift32[x_] := Module[{y = BitAnd[BitXor[x, BitShiftLeft[x, 13]], 2^32 - 1]},
  y = BitAnd[BitXor[y, BitShiftRight[y, 17]], 2^32 - 1];
  BitAnd[BitXor[y, BitShiftLeft[y, 5]], 2^32 - 1]];

dir = "/home/hatch/workspace/ghost/native/c/run2";
num[line_, key_] := Module[{p, rest, chars, out = ""},
  p = StringPosition[line, "\"" <> key <> "\":"];
  If[p === {}, Return[0]];
  rest = StringDrop[line, p[[1, 2]]];
  chars = Characters[rest];
  Do[If[MemberQ[{"0","1","2","3","4","5","6","7","8","9"}, chars[[k]]],
       out = out <> chars[[k]],
       If[out =!= "" || chars[[k]] =!= " ", Break[]]], {k, Length[chars]}];
  If[out === "", 0, ToExpression[out]]];
parseLine[line_] :=
  If[StringContainsQ[line, "\"Store\""],
    <|"Op" -> "Store", "Addr" -> num[line, "Addr"],
      "Width" -> num[line, "Width"], "Val" -> num[line, "Val"]|>,
    <|"Op" -> "Load", "Addr" -> num[line, "Addr"],
      "Width" -> num[line, "Width"]|>];
readLines[f_] := Select[StringSplit[Import[f, "Text"], "\n"], # != "" &];

imageC = BinaryReadList[dir <> "/mem.bin"];
traceC = parseLine /@ readLines[dir <> "/trace.jsonl"];
Print["trace parsed: ", Length[traceC]];

s = 20261031;
next[] := (s = XorShift32[s]);
imageW = Table[BitAnd[next[], 255], {1024}];
Print["B initial-memory-equal: ", imageW === imageC];
traceW = Table[
  Module[{a = Mod[next[], 1016], w = 2^Mod[next[], 3], op = Mod[next[], 3]},
    If[op < 2,
      <|"Op" -> "Load", "Addr" -> a, "Width" -> w|>,
      Module[{v = BitOr[BitShiftLeft[next[], 32], next[]]},
        <|"Op" -> "Store", "Addr" -> a, "Width" -> w,
          "Val" -> BitAnd[v, 2^(8 w) - 1]|>]]],
  {500}];
opMismatch = Count[Table[traceW[[i]] =!= traceC[[i]], {i, 500}], True];
Print["A prng-stream trace ops compared: 500, mismatches: ", opMismatch];
firstBad = Select[Range[500], traceW[[#]] =!= traceC[[#]] &];
firstBad = Take[firstBad, Min[3, Length[firstBad]]];
Print["A first mismatching op indices: ", firstBad];
If[firstBad =!= {},
  Print["A example wolfram: ", ToString[traceW[[firstBad[[1]]]], InputForm]];
  Print["A example C: ", ToString[traceC[[firstBad[[1]]]], InputForm]]];
Export["/home/hatch/workspace/ghost/native/c/run2/imageW.m", imageW];
