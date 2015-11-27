## GCN ISA VOP1/VOP3 instructions

VOP1 instructions can be encoded in the VOP1 encoding and the VOP3A/VOP3B encoding.
List of fields for VOP1 encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-8   | SRC0     | First (scalar or vector) source operand
9-16  | OPCODE   | Operation code
17-24 | VDST     | Destination vector operand
25-31 | ENCODING | Encoding type. Must be 0b0111111

Syntax: INSTRUCTION VDST, SRC0

List of fields for VOP3A/VOP3B encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Vector destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
11    | CLAMP    | CLAMP modifier (VOP3A)
15    | CLAMP    | CLAMP modifier (VOP3B)
17-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

List of fields for VOP3A/VOP3B encoding (GCN 1.2):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Destination vector operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
15    | CLAMP    | CLAMP modifier
16-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

Syntax: INSTRUCTION VDST, SRC0 [MODIFIERS]

Modifiers:

* CLAMP - clamps destination floating point value in range 0.0-1.0
* MUL:2, MUL:4, DIV:2 - OMOD modifiers. Multiply destination floating point value by
2.0, 4.0 or 0.5 respectively
* -SRC - negate floating point value from source operand
* ABS(SRC) - apply absolute value to source operand

Negation and absolute value can be combined: `-ABS(V0)`. Modifiers CLAMP and
OMOD (MUL:2, MUL:4 and DIV:2) can be given in random order.

Limitations for operands:

* only one SGPR can be read by instruction. Multiple occurrences of this same
SGPR is allowed
* only one literal constant can be used, and only when a SGPR or M0 is not used in
source operands
* only SRC0 can holds LDS_DIRECT

VOP1 opcodes (0-127) are reflected in VOP3 in range: 384-511 for GCN 1.0/1.1 or
320-447 for GCN 1.2.

List of the instructions by opcode (GCN 1.0/1.1):

 Opcode     | Opcode(VOP3)|GCN 1.0|GCN 1.1| Mnemonic
------------|-------------|-------|-------|-----------------------------
 0 (0x0)    | 384 (0x180) |   ✓   |   ✓   | V_NOP
 1 (0x1)    | 385 (0x181) |   ✓   |   ✓   | V_MOV_B32
 2 (0x2)    | 386 (0x182) |   ✓   |   ✓   | V_READFIRSTLANE_B32
 3 (0x3)    | 387 (0x183) |   ✓   |   ✓   | V_CVT_I32_F64
 4 (0x4)    | 388 (0x184) |   ✓   |   ✓   | V_CVT_F64_I32
 5 (0x5)    | 389 (0x185) |   ✓   |   ✓   | V_CVT_F32_I32
 6 (0x6)    | 390 (0x186) |   ✓   |   ✓   | V_CVT_F32_U32
 7 (0x7)    | 391 (0x187) |   ✓   |   ✓   | V_CVT_U32_F32
 8 (0x8)    | 392 (0x188) |   ✓   |   ✓   | V_CVT_I32_F32
 9 (0x9)    | 393 (0x189) |   ✓   |   ✓   | V_MOV_FED_B32
 10 (0xa)   | 394 (0x18a) |   ✓   |   ✓   | V_CVT_F16_F32
 11 (0xb)   | 395 (0x18b) |   ✓   |   ✓   | V_CVT_F32_F16
 12 (0xc)   | 396 (0x18c) |   ✓   |   ✓   | V_CVT_RPI_I32_F32
 13 (0xd)   | 397 (0x18d) |   ✓   |   ✓   | V_CVT_FLR_I32_F32
 14 (0xe)   | 398 (0x18e) |   ✓   |   ✓   | V_CVT_OFF_F32_I4
 15 (0xf)   | 399 (0x18f) |   ✓   |   ✓   | V_CVT_F32_F64
 16 (0x10)  | 400 (0x190) |   ✓   |   ✓   | V_CVT_F64_F32
 17 (0x11)  | 401 (0x191) |   ✓   |   ✓   | V_CVT_F32_UBYTE0
 18 (0x12)  | 402 (0x192) |   ✓   |   ✓   | V_CVT_F32_UBYTE1
 19 (0x13)  | 403 (0x193) |   ✓   |   ✓   | V_CVT_F32_UBYTE2
 20 (0x14)  | 404 (0x194) |   ✓   |   ✓   | V_CVT_F32_UBYTE3
 21 (0x15)  | 405 (0x195) |   ✓   |   ✓   | V_CVT_U32_F64
 22 (0x16)  | 406 (0x196) |   ✓   |   ✓   | V_CVT_F64_U32
 23 (0x17)  | 407 (0x197) |       |   ✓   | V_TRUNC_F64
 24 (0x18)  | 408 (0x198) |       |   ✓   | V_CEIL_F64
 25 (0x19)  | 409 (0x199) |       |   ✓   | V_RNDNE_F64
 26 (0x1a)  | 410 (0x19a) |       |   ✓   | V_FLOOR_F64
 32 (0x20)  | 416 (0x1a0) |   ✓   |   ✓   | V_FRACT_F32
 33 (0x21)  | 417 (0x1a1) |   ✓   |   ✓   | V_TRUNC_F32
 34 (0x22)  | 418 (0x1a2) |   ✓   |   ✓   | V_CEIL_F32
 35 (0x23)  | 419 (0x1a3) |   ✓   |   ✓   | V_RNDNE_F32
 36 (0x24)  | 420 (0x1a4) |   ✓   |   ✓   | V_FLOOR_F32
 37 (0x25)  | 421 (0x1a5) |   ✓   |   ✓   | V_EXP_F32
 38 (0x26)  | 422 (0x1a6) |   ✓   |   ✓   | V_LOG_CLAMP_F32
 39 (0x27)  | 423 (0x1a7) |   ✓   |   ✓   | V_LOG_F32
 40 (0x28)  | 424 (0x1a8) |   ✓   |   ✓   | V_RCP_CLAMP_F32
 41 (0x29)  | 425 (0x1a9) |   ✓   |   ✓   | V_RCP_LEGACY_F32
 42 (0x2a)  | 426 (0x1aa) |   ✓   |   ✓   | V_RCP_F32
 43 (0x2b)  | 427 (0x1ab) |   ✓   |   ✓   | V_RCP_IFLAG_F32
 44 (0x2c)  | 428 (0x1ac) |   ✓   |   ✓   | V_RSQ_CLAMP_F32
 45 (0x2d)  | 429 (0x1ad) |   ✓   |   ✓   | V_RSQ_LEGACY_F32
 46 (0x2e)  | 430 (0x1ae) |   ✓   |   ✓   | V_RSQ_F32
 47 (0x2f)  | 431 (0x1af) |   ✓   |   ✓   | V_RCP_F64
 48 (0x30)  | 432 (0x1b0) |   ✓   |   ✓   | V_RCP_CLAMP_F64
 49 (0x31)  | 433 (0x1b1) |   ✓   |   ✓   | V_RSQ_F64
 50 (0x32)  | 434 (0x1b2) |   ✓   |   ✓   | V_RSQ_CLAMP_F64
 51 (0x33)  | 435 (0x1b3) |   ✓   |   ✓   | V_SQRT_F32
 52 (0x34)  | 436 (0x1b4) |   ✓   |   ✓   | V_SQRT_F64
 53 (0x35)  | 437 (0x1b5) |   ✓   |   ✓   | V_SIN_F32
 54 (0x36)  | 438 (0x1b6) |   ✓   |   ✓   | V_COS_F32
 55 (0x37)  | 439 (0x1b7) |   ✓   |   ✓   | V_NOT_B32
 56 (0x38)  | 440 (0x1b8) |   ✓   |   ✓   | V_BFREV_B32
 57 (0x39)  | 441 (0x1b9) |   ✓   |   ✓   | V_FFBH_U32
 58 (0x3a)  | 442 (0x1ba) |   ✓   |   ✓   | V_FFBL_B32
 59 (0x3b)  | 443 (0x1bb) |   ✓   |   ✓   | V_FFBH_I32
 60 (0x3c)  | 444 (0x1bc) |   ✓   |   ✓   | V_FREXP_EXP_I32_F64
 61 (0x3d)  | 445 (0x1bd) |   ✓   |   ✓   | V_FREXP_MANT_F64
 62 (0x3e)  | 446 (0x1be) |   ✓   |   ✓   | V_FRACT_F64
 63 (0x3f)  | 447 (0x1bf) |   ✓   |   ✓   | V_FREXP_EXP_I32_F32
 64 (0x40)  | 448 (0x1c0) |   ✓   |   ✓   | V_FREXP_MANT_F32
 65 (0x41)  | 449 (0x1c1) |   ✓   |   ✓   | V_CLREXCP
 66 (0x42)  | 450 (0x1c2) |   ✓   |   ✓   | V_MOVRELD_B32
 67 (0x43)  | 451 (0x1c3) |   ✓   |   ✓   | V_MOVRELS_B32
 68 (0x44)  | 452 (0x1c4) |   ✓   |   ✓   | V_MOVRELSD_B32
 69 (0x45)  | 453 (0x1c5) |       |   ✓   | V_LOG_LEGACY_F32
 70 (0x46)  | 454 (0x1c6) |       |   ✓   | V_EXP_LEGACY_F32

List of the instructions by opcode (GCN 1.2):

 Opcode     | Opcode(VOP3)| Mnemonic
------------|-------------|-----------------------------
 0 (0x0)    | 320 (0x140) | V_NOP
 1 (0x1)    | 321 (0x141) | V_MOV_B32
 2 (0x2)    | 322 (0x142) | V_READFIRSTLANE_B32
 3 (0x3)    | 323 (0x143) | V_CVT_I32_F64
 4 (0x4)    | 324 (0x144) | V_CVT_F64_I32
 5 (0x5)    | 325 (0x145) | V_CVT_F32_I32
 6 (0x6)    | 326 (0x146) | V_CVT_F32_U32
 7 (0x7)    | 327 (0x147) | V_CVT_U32_F32
 8 (0x8)    | 328 (0x148) | V_CVT_I32_F32
 9 (0x9)    | 329 (0x149) | V_MOV_FED_B32
 10 (0xa)   | 330 (0x14a) | V_CVT_F16_F32
 11 (0xb)   | 331 (0x14b) | V_CVT_F32_F16
 12 (0xc)   | 332 (0x14c) | V_CVT_RPI_I32_F32
 13 (0xd)   | 333 (0x14d) | V_CVT_FLR_I32_F32
 14 (0xe)   | 334 (0x14e) | V_CVT_OFF_F32_I4
 15 (0xf)   | 335 (0x14f) | V_CVT_F32_F64
 16 (0x10)  | 336 (0x150) | V_CVT_F64_F32
 17 (0x11)  | 337 (0x151) | V_CVT_F32_UBYTE0
 18 (0x12)  | 338 (0x152) | V_CVT_F32_UBYTE1
 19 (0x13)  | 339 (0x153) | V_CVT_F32_UBYTE2
 20 (0x14)  | 340 (0x154) | V_CVT_F32_UBYTE3
 21 (0x15)  | 341 (0x155) | V_CVT_U32_F64
 22 (0x16)  | 342 (0x156) | V_CVT_F64_U32
 23 (0x17)  | 343 (0x157) | V_TRUNC_F64
 24 (0x18)  | 344 (0x158) | V_CEIL_F64
 25 (0x19)  | 345 (0x159) | V_RNDNE_F64
 26 (0x1a)  | 346 (0x15a) | V_FLOOR_F64
 27 (0x1b)  | 347 (0x15b) | V_FRACT_F32
 28 (0x1c)  | 348 (0x15c) | V_TRUNC_F32
 29 (0x1d)  | 349 (0x15d) | V_CEIL_F32
 30 (0x1e)  | 350 (0x15e) | V_RNDNE_F32
 31 (0x1f)  | 351 (0x15f) | V_FLOOR_F32
 32 (0x20)  | 352 (0x160) | V_EXP_F32
 33 (0x21)  | 353 (0x161) | V_LOG_F32
 34 (0x22)  | 354 (0x162) | V_RCP_F32
 35 (0x23)  | 355 (0x163) | V_RCP_IFLAG_F32
 36 (0x24)  | 356 (0x164) | V_RSQ_F32
 37 (0x25)  | 357 (0x165) | V_RCP_F64
 38 (0x26)  | 358 (0x166) | V_RSQ_F64
 39 (0x27)  | 359 (0x167) | V_SQRT_F32
 40 (0x28)  | 360 (0x168) | V_SQRT_F64
 41 (0x29)  | 361 (0x169) | V_SIN_F32
 42 (0x2a)  | 362 (0x16a) | V_COS_F32
 43 (0x2b)  | 363 (0x16b) | V_NOT_B32
 44 (0x2c)  | 364 (0x16c) | V_BFREV_B32
 45 (0x2d)  | 365 (0x16d) | V_FFBH_U32
 46 (0x2e)  | 366 (0x16e) | V_FFBL_B32
 47 (0x2f)  | 367 (0x16f) | V_FFBH_I32
 48 (0x30)  | 368 (0x170) | V_FREXP_EXP_I32
 49 (0x31)  | 369 (0x171) | V_FREXP_MANT_F6
 50 (0x32)  | 370 (0x172) | V_FRACT_F64
 51 (0x33)  | 371 (0x173) | V_FREXP_EXP_I32
 52 (0x34)  | 372 (0x174) | V_FREXP_MANT_F3
 53 (0x35)  | 373 (0x175) | V_CLREXCP
 54 (0x36)  | 374 (0x176) | V_MOVRELD_B32
 55 (0x37)  | 375 (0x177) | V_MOVRELS_B32
 56 (0x38)  | 376 (0x178) | V_MOVRELSD_B32
 57 (0x39)  | 377 (0x179) | V_CVT_F16_U16
 58 (0x3a)  | 378 (0x17a) | V_CVT_F16_I16
 59 (0x3b)  | 379 (0x17b) | V_CVT_U16_F16
 60 (0x3c)  | 380 (0x17c) | V_CVT_I16_F16
 61 (0x3d)  | 381 (0x17d) | V_RCP_F16
 62 (0x3e)  | 382 (0x17e) | V_SQRT_F16
 63 (0x3f)  | 383 (0x17f) | V_RSQ_F16
 64 (0x40)  | 384 (0x180) | V_LOG_F16
 65 (0x41)  | 385 (0x181) | V_EXP_F16
 66 (0x42)  | 386 (0x182) | V_FREXP_MANT_F16
 67 (0x43)  | 387 (0x183) | V_FREXP_EXP_I16_F16
 68 (0x44)  | 388 (0x184) | V_FLOOR_F16
 69 (0x45)  | 389 (0x185) | V_CEIL_F16
 70 (0x46)  | 390 (0x186) | V_TRUNC_F16
 71 (0x47)  | 391 (0x187) | V_RNDNE_F16
 72 (0x48)  | 392 (0x188) | V_FRACT_F16
 73 (0x49)  | 393 (0x189) | V_SIN_F16
 74 (0x4a)  | 394 (0x18a) | V_COS_F16
 75 (0x4b)  | 395 (0x18b) | V_EXP_LEGACY_F32
 76 (0x4c)  | 396 (0x18c) | V_LOG_LEGACY_F32

### Instruction set

Alphabetically sorted instruction list:

#### V_CVT_F16_F32

Opcode VOP2: 10 (0xa)  
Opcode VOP3A: 394 (0x18a) for GCN 1.0/1.1; 330 (0x14a) for GCN 1.2  
Syntax: V_CVT_F16_F32 VDST, SRC0  
Description: Convert single FP value to half floating point value with rounding from
MODE register (single FP rounding mode), and store result to VDST.
If absolute value is too high, then store -/+infinity to VDST.  
Operation:  
```
VDST = RNDHALF(ASFLOAT(SRC0))
```

#### V_CVT_F32_F16

Opcode VOP2: 11 (0xb)  
Opcode VOP3A: 395 (0x18b) for GCN 1.0/1.1; 331 (0x14b) for GCN 1.2  
Syntax: V_CVT_F32_F16 VDST, SRC0  
Description: Convert half FP value to single FP value, and store result to VDST.  
Operation:  
```
VDST = (FLOAT)(ASHALF(SRC0))
```

#### V_CVT_F32_I32

Opcode VOP2: 5 (0x5)  
Opcode VOP3A: 389 (0x185) for GCN 1.0/1.1; 325 (0x145) for GCN 1.2  
Syntax: V_CVT_F32_I32 VDST, SRC0  
Description: Convert signed 32-bit integer to single FP value, and store it to VDST.  
Operation:  
```
VDST = (FLOAT)(INT32)SRC0
```

#### V_CVT_F32_U32

Opcode VOP2: 6 (0x6)  
Opcode VOP3A: 390 (0x186) for GCN 1.0/1.1; 326 (0x146) for GCN 1.2  
Syntax: V_CVT_F32_U32 VDST, SRC0  
Description: Convert unsigned 32-bit integer to single FP value, and store it to VDST.  
Operation:  
```
VDST = (FLOAT)SRC0

#### V_CVT_F64_I32

Opcode VOP2: 4 (0x4)  
Opcode VOP3A: 388 (0x184) for GCN 1.0/1.1; 324 (0x144) for GCN 1.2  
Syntax: V_CVT_F64_I32 VDST(2), SRC0  
Description: Convert signed 32-bit integer to double FP value, and store it to VDST.  
Operation:  
```
VDST = (DOUBLE)(INT32)SRC0
```

#### V_CVT_FLR_I32_F32

Opcode VOP2: 13 (0xd)  
Opcode VOP3A: 397 (0x18d) for GCN 1.0/1.1; 333 (0x14d) for GCN 1.2  
Syntax: V_CVT_FLR_I32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion uses rounding to negative infinity (floor).
If value is higher/lower than maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN/-NaN then store MAX_INT32/MIN_INT32 to VDST.  
Operation:  
```
if (ABS(SRC0)!=NAN)
    VDST = (INT32)MAX(MIN(FLOOR(ASFLOAT(SRC0)), 2147483647.0), -2147483648.0)
else
    VDST = (INT32)SRC0>=0 ? 2147483647 : -2147483648
```

#### V_CVT_I32_F32

Opcode VOP2: 8 (0x8)  
Opcode VOP3A: 392 (0x188) for GCN 1.0/1.1; 328 (0x148) for GCN 1.2  
Syntax: V_CVT_I32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher/lower than
maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (SRC0!=NAN)
    VDST = (INT32)MAX(MIN(RNDTZINT(ASFLOAT(SRC0)), 2147483647.0), -2147483648.0)
```

#### V_CVT_I32_F64

Opcode VOP2: 3 (0x3)  
Opcode VOP3A: 387 (0x183) for GCN 1.0/1.1; 323 (0x143) for GCN 1.2  
Syntax: V_CVT_I32_F64 VDST, SRC0(2)  
Description: Convert 64-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher/lower than
maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (SRC0!=NAN)
    VDST = (INT32)MAX(MIN(RNDTZINT(ASDOUBLE(SRC0)), 2147483647.0), -2147483648.0)
```

#### V_CVT_RPI_I32_F32

Opcode VOP2: 12 (0xc)  
Opcode VOP3A: 396 (0x18c) for GCN 1.0/1.1; 332 (0x14c) for GCN 1.2  
Syntax: V_CVT_RPI_I32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to signed 32-bit integer, and
store result to VDST. Conversion adds 0.5 to value and rounds negative infinity (floor).
If value is higher/lower than maximal/minimal integer then store MAX_INT32/MIN_INT32 to VDST.
If input value is NaN/-NaN then store MAX_INT32/MIN_INT32 to VDST.  
Description:  
```
if (ABS(SRC0)!=NAN)
    VDST = (INT32)MAX(MIN(FLOOR(ASFLOAT(SRC0) + 0.5), 2147483647.0), -2147483648.0)
else
    VDST = (INT32)SRC0>=0 ? 2147483647 : -2147483648
```

#### V_CVT_U32_F32

Opcode VOP2: 7 (0x7)  
Opcode VOP3A: 391 (0x187) for GCN 1.0/1.1; 327 (0x147) for GCN 1.2  
Syntax: V_CVT_U32_F32 VDST, SRC0  
Description: Convert 32-bit floating point value from SRC0 to unsigned 32-bit integer, and
store result to VDST. Conversion uses rounding to zero. If value is higher than
maximal integer then store MAX_UINT32 to VDST.
If input value is NaN then store 0 to VDST.  
Operation:  
```
VDST = 0
if (SRC0!=NAN)
    VDST = (UINT32)MIN(RNDTZINT(ASFLOAT(SRC0)), 4294967295.0)
```

#### V_MOV_FED_B32

Opcode VOP2: 9 (0x9)  
Opcode VOP3A: 393 (0x189) for GCN 1.0/1.1; 329 (0x149) for GCN 1.2  
Syntax: V_MOV_FED_B32 VDST, SRC0  
Description: Introduce edc double error upon write to dest vgpr without causing an exception
(???).

#### V_MOV_B32

Opcode VOP2: 1 (0x1)  
Opcode VOP3A: 385 (0x181) for GCN 1.0/1.1; 321 (0x141) for GCN 1.2  
Syntax: V_MOV_B32 VDST, SRC0  
Description: Move SRC0 into VDST.  
Operation:  
```
VDST = SRC0
```

#### V_NOP

Opcode VOP2: 0 (0x0)  
Opcode VOP3A: 384 (0x180) for GCN 1.0/1.1; 320 (0x140) for GCN 1.2  
Syntax: V_NOP  
Description: Do nothing.

#### V_READFIRSTLANE_B32

Opcode VOP2: 2 (0x2)  
Opcode VOP3A: 386 (0x182) for GCN 1.0/1.1; 322 (0x142) for GCN 1.2  
Syntax: V_READFIRSTLANE_B32 SDST, VSRC0  
Description: Copy one VSRC0 lane value to one SDST. Lane (thread id) is first active lane id
or first lane id all lanes are inactive. SSRC1 can be SGPR or M0. Ignores EXEC mask.  
Operation:  
Operation:  
```
UINT8 firstlane = 0
for (UINT8 i = 0; i < 64; i++)
    if ((1ULL<<i) & EXEC) != 0)
    { firstlane = i; break; }
SDST = VSRC0[firstlane]
```