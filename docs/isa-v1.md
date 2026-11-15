# HAEUN-32 ISA specification, version 1

Status: frozen initial contract for the implemented integer subset. All multibyte instruction and data values are little-endian. Instruction addresses and branch destinations are 4-byte aligned. The VM checks instruction fetch bounds and word data alignment/range.

## Architectural state

- `r0`–`r15`: sixteen unsigned 32-bit integer registers. `r0` reads as zero and writes are discarded after every successfully completed instruction.
- `PC`: 32-bit byte address of the current instruction.
- `cause`, `fault_pc`: VM diagnostic state, not guest-writable registers in v1.
- No flags, floating point, privilege level, CSR, or interrupt state is defined in v1.

## Instruction word and formats

Every instruction is one 32-bit word. `op` occupies bits 31:26, `rd` bits 25:22, `ra` bits 21:18, `rb` bits 17:14, and `imm18` bits 17:0. Register fields are four bits. R-format uses `op, rd, ra, rb`; low 14 bits are reserved and must be zero. I-format uses `op, rd, ra, imm18`. Branch format uses `op, rd, ra, imm18`; `rd` and `ra` are compared. JAL uses `op, rd, imm18`; unused register fields are reserved zero. NOP and HALT require all non-op bits zero. Malformed encodings trap as illegal.

`imm18` is signed two's-complement, sign-extended to 32 bits. For branch and jump the signed immediate is a count of instruction words relative to `PC+4`. For load/store it is a byte offset from `r[ra]`.

## Opcode table

| op | Mnemonic | Semantics |
|---:|---|---|
| 0x00 | NOP | no state change except `PC := PC+4` |
| 0x01 | HALT | set halted state; `PC := PC+4`; VM returns HALTED |
| 0x02 | ADD | `rd := ra + rb` modulo 2^32 |
| 0x03 | SUB | `rd := ra - rb` modulo 2^32 |
| 0x04 | AND | bitwise and |
| 0x05 | OR | bitwise or |
| 0x06 | XOR | bitwise xor |
| 0x07 | ADDI | `rd := ra + signext(imm18)` modulo 2^32 |
| 0x08 | LW | load little-endian 32-bit word from `ra + signext(imm18)` |
| 0x09 | SW | store `rd` to little-endian 32-bit word at `ra + signext(imm18)` |
| 0x0a | BEQ | if `rd == ra`, `PC := PC+4 + signext(imm18)*4`; else `PC+4` |
| 0x0b | BNE | if `rd != ra`, same target rule as BEQ |
| 0x0c | JAL | `rd := PC+4`; `PC := PC+4 + signext(imm18)*4` |
| 0x0d | ECALL | synchronous environment-call trap, cause 3; VM stops because v1 has no handler mechanism |

Unused opcodes, including `0x0e` and `0x0f`, and encodings with nonzero reserved fields are illegal instruction traps in v1. `r0` write suppression applies to link and load destinations too. ISA v2 assigns opcode `0x0e` to JALR and ISA v3 assigns `0x0f` to CMP without changing this v1 contract. The implementation enforces v1 reserved bits and reports ECALL with a distinct trap stop result.

## Fault and control-transfer behavior

On an instruction fetch or data fault, the VM leaves `PC` at the faulting instruction, records `fault_pc=PC`, records a numeric cause, and returns a stop reason. Cause 0 is illegal opcode/encoding, 1 is address out of range, 2 is alignment fault, 3 is ECALL. No architectural trap vector or handler exists in v1. A subsequent caller may inspect state; the VM does not silently advance past a fault.

`HALT` advances PC and makes later steps return HALTED. `ECALL` returns the distinct VM_TRAP stop result and cause 3; no handler exists. `run` stops at the first non-OK step result or after the caller-supplied instruction budget. Control-flow targets are checked on the next fetch, so an out-of-range branch records the target as PC and faults on that fetch.

## Opcode encoding test vectors

All are single-word values, with registers shown as numeric indices: `NOP = 0x00000000`, `HALT = 0x04000000`, `ADD r3,r1,r2 = 0x08C48000`, `ADDI r3,r1,-1 = 0x1CC7FFFF`, `LW r3,4(r1) = 0x20C40004`, `BEQ r1,r2,+1 = 0x28480001`, `JAL r5,+2 = 0x31400002`. `tests/vm_tests.c` encodes each supported opcode and exercises it.

## Compatibility

Images declare ISA version 1. Version 1 opcode meanings and encoding are stable. Future incompatible changes require a distinct major ISA version; compatible optional extensions must use reserved opcode space and be advertised by a machine capability record. No extension mechanism is implemented yet.
