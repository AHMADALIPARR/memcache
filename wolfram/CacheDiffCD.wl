(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* CacheDiffCD.wl -- Directive 003 section 4, checks C and D:
   value differential by load sequence number + final memory equality.
   Replays trace.jsonl over the mem.bin image in Wolfram and compares
   against the C replay's results_replay.jsonl and final.bin. *)
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

m = BinaryReadList[dir <> "/mem.bin"];
traceC = parseLine /@ readLines[dir <> "/trace.jsonl"];
resultsC = num[#, "Value"] & /@ readLines[dir <> "/results_replay.jsonl"];
finalC = BinaryReadList[dir <> "/final.bin"];
Print["read: trace ", Length[traceC], ", results ", Length[resultsC]];

li = 0; mism = {};
Do[
  Which[
    t["Op"] === "Load",
    li++;
    v = FromDigits[Reverse[Take[m, {t["Addr"] + 1, t["Addr"] + t["Width"]}]], 256];
    If[v =!= resultsC[[li]],
      mism = Join[mism, {<|"Load#" -> li, "Addr" -> t["Addr"],
        "Width" -> t["Width"], "Wolfram" -> v, "C" -> resultsC[[li]]|>}]],
    t["Op"] === "Store",
    Do[m[[t["Addr"] + i]] = BitAnd[BitShiftRight[t["Val"], 8 (i - 1)], 255],
      {i, t["Width"]}]],
  {t, traceC}];
Print["C loads compared: ", li, ", mismatches: ", Length[mism]];
Print["C first mismatches: ", Take[mism, Min[5, Length[mism]]]];
Print["D final-memory-equal: ", m === finalC];
diffBytes = Count[Table[m[[i]] =!= finalC[[i]], {i, 1024}], True];
Print["D differing final bytes: ", diffBytes];
