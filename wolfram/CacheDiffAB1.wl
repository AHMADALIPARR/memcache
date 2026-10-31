(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* CacheDiffAB1.wl -- Directive 003 section 4, check B (stage 1 of 2):
   regenerate the 1024 image draws of the driver stream in Wolfram and
   compare against mem.bin. Exports the post-image stream state for
   stage 2. Bit operations are expressed arithmetically (IntegerDigits
   vector form): Mathics3 here evaluates none of the Bit* builtins. *)
xorD[a_, b_] := FromDigits[Mod[IntegerDigits[a, 2, 32] +
  IntegerDigits[b, 2, 32], 2], 2];
XorShift32A[x_] := Module[{y = Mod[xorD[x, x * 2^13], 2^32]},
  y = Mod[xorD[y, Quotient[y, 2^17]], 2^32];
  Mod[xorD[y, y * 2^5], 2^32]];
readBytes[path_] := Module[{st = OpenRead[path, BinaryFormat -> True], data},
  data = Table[Read[st, Byte], {1024}]; Close[st]; data];

imageC = readBytes["/home/hatch/workspace/ghost/native/c/run2/mem.bin"];
s = 20261031;
imageW = Table[s = XorShift32A[s]; Mod[s, 256], {1024}];
Print["B initial-memory-equal: ", imageW === imageC];
Print["B differing bytes: ", Count[Table[imageW[[i]] =!= imageC[[i]], {i, 1024}], True]];
Export["/home/hatch/workspace/ghost/native/c/run2/stream_state.m", s];
Print["state exported"];
