(* Copyright (c) 2026 Nova Parr, Ahmad Ali Parr, and Team HoloTwinKitty *)
VerificationTest[
  XorShift32[1] == 270369,
  {1, 270369, XorShift32[270369]} // First
]

VerificationTest[
  FromDigits[Reverse[{0xEF, 0xBE}], 256] == 0xBEEF
]
