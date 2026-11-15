# HAEUN-32 ISA version 2 extension

ISA v2 preserves every v1 instruction and encoding. It assigns one previously reserved opcode, `0x0e`, to register-indirect jump-and-link. An image's HXF `isa_version` field selects the contract. ISA v1 execution continues to trap on opcode `0x0e`; the same word executes as JALR only when the VM is explicitly initialized for ISA v2.

## JALR

JALR uses the immediate format: `op[31:26]=0x0e`, `rd[25:22]`, `ra[21:18]`, and signed byte displacement `imm18[17:0]`.

1. Compute `target := r[ra] + signext(imm18)` modulo 2^32 before changing any register.
2. Write `PC+4` to `r[rd]`; writes to `r0` are discarded.
3. Set `PC := target`.

If `rd == ra`, target uses the old source value. Target alignment and range are checked at the next instruction fetch, consistent with v1 direct branches. No target bits are masked. JALR has no reserved operand bits.

`RET` is an assembler pseudoinstruction, not a separate opcode: `jalr r0, r15, 0`. Calls use `jal r15, symbol` while the target is within the signed PC-relative range; returns use RET. There is still no privilege mode, trap vector, or interrupt-return instruction.

## Image and toolchain versioning

HXF container format version remains 1. Its `isa_version` field is 2 for v2 guest code. HOBJ v3 records ISA version 2 in its header; HOBJ v1/v2 remain ISA v1. The linker rejects mixed-ISA object sets. See [HOBJ v3](hobj-v3.md). ISA v3 later adds opcode `0x0f` without changing v2 meanings.

The C VM exposes explicit ISA-version initialization. The legacy initializer selects v1. The legacy HXF loader is v1-only; the version-aware loader returns the image ISA version so callers cannot silently run v2 code under v1 rules.

## Verification

Tests verify that opcode `0x0e` traps under v1, JALR transfers and links under v2, RET returns through `r15`, and misaligned JALR targets fault at fetch. The end-to-end HOBJ v3/HXF v1-format test links and runs a v2 call/return guest program.
