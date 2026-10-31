(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* CacheDiffAB2.wl -- Directive 003 section 4, check A (stage 2 of 2):
   continue the stream from the exported post-image state, regenerate
   the 500 driver operations in the driver's draw order, and compare
   against trace.jsonl field by field. *)
xorD[a_, b_] := FromDigits[Mod[IntegerDigits[a, 2, 32] +
  IntegerDigits[b, 2, 32], 2], 2];
XorShift32A[x_] := Module[{y = Mod[xorD[x, x * 2^13], 2^32]},
  y = Mod[xorD[y, Quotient[y, 2^17]], 2^32];
  Mod[xorD[y, y * 2^5], 2^32]];
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

dir = "/home/hatch/workspace/ghost/native/c/run2";
traceC = parseLine /@ readLines[dir <> "/trace.jsonl"];
s = 20261031;
Do[s = XorShift32A[s], {1024}];   (* replay the image draws *)
next[] := (s = XorShift32A[s]);
traceW = Table[
  Module[{a = Mod[next[], 1016], w = 2^Mod[next[], 3], op = Mod[next[], 3]},
    If[op < 2,
      <|"Op" -> "Load", "Addr" -> a, "Width" -> w|>,
      Module[{v = next[] * 2^32 + next[]},
        <|"Op" -> "Store", "Addr" -> a, "Width" -> w,
          "Val" -> Mod[v, 2^(8 w)]|>]]],
  {500}];
opMismatch = Count[Table[traceW[[i]] =!= traceC[[i]], {i, 500}], True];
Print["A prng-stream trace ops compared: 500, mismatches: ", opMismatch];
firstBad = Select[Range[500], traceW[[#]] =!= traceC[[#]] &];
firstBad = Take[firstBad, Min[3, Length[firstBad]]];
Print["A first mismatching op indices: ", firstBad];
If[firstBad =!= {},
  Print["A example wolfram: ", ToString[traceW[[firstBad[[1]]]], InputForm]];
  Print["A example C: ", ToString[traceC[[firstBad[[1]]]], InputForm]]];
