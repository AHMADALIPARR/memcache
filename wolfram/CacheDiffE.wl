(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
(* CacheDiffE.wl -- Directive 003 section 5: the Valid invariant in
   Wolfram form, tested for equivalence over the dumped C grid
   (range_grid.csv: addr,width,capacity,range_valid).
   The invariant Valid(a,w,M) = 0<=a<=M && 0<w<=M-a corresponds on the
   C side to range_valid(a,w,M) && w>0: range_valid alone admits
   width 0, and the 0<w conjunct is the separate width layer of the
   contract (the same split the C bounds test asserts). *)
CacheValid[a_, w_, m_] := 0 <= a <= m && 0 < w <= m - a;
rows = Map[ToExpression, Import[
  "/home/hatch/workspace/ghost/native/c/run2/range_grid.csv", "CSV"], {2}];
bad = Select[rows, CacheValid[#[[1]], #[[2]], #[[3]]] =!=
  ((#[[4]] === 1) && #[[2]] > 0) &];
Print["E range-grid rows: ", Length[rows], ", disagreements: ", Length[bad]];
Print["E first disagreements: ", Take[bad, Min[5, Length[bad]]]];
