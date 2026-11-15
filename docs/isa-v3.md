# HAEUN-32 ISA version 3 signed comparisons

ISA v3 preserves every v1 and v2 instruction encoding and assigns opcode `0x0f` to `CMP`. Images declare ISA version 3 in HXF; a v1/v2 VM must reject opcode `0x0f` as illegal. This is an ISA extension, not a change to the HXF container format.

## Encoding

The 32-bit instruction uses the standard register fields: `op[31:26]=0x0f`, `rd[25:22]`, `ra[21:18]`, and `rb[17:14]`. Bits `13:11` contain a three-bit condition code; bits `10:0` are reserved and must be zero. `CMP` writes exactly 0 or 1 to `rd`, subject to the architectural `r0` write suppression.

| Condition | Meaning |
|---:|---|
| 0 | equal: unsigned bit patterns equal |
| 1 | not equal |
| 2 | signed less than |
| 3 | signed less than or equal |
| 4 | signed greater than |
| 5 | signed greater than or equal |
| 6–7 | illegal encoding |

Signed comparisons interpret each 32-bit operand as a two's-complement signed integer. Implementations must avoid relying on host signed overflow or on implementation-defined conversion of out-of-range unsigned values to signed values. Conditions 6 and 7, or any nonzero reserved bit, produce the illegal-instruction fault without retiring the instruction.

## Compatibility and verification

Opcode `0x0f` remains illegal for ISA v1 and v2. HOBJ v4 records ISA v3, while HOBJ v1/v2/v3 retain their existing contracts. HXF remains format v1 and carries ISA version 3 in its existing header field. All six conditions, invalid encodings, assembler mnemonics, compiler relational expressions, and cross-version rejection are exercised by the VM/toolchain tests.
