# HAEUN-32 ISA (architecture index)

This document is the architecture entry point for the HAEUN-32 CPU emulator. Normative details live in the versioned specs below; this file consolidates the opcode map, module layout, and verification expectations.

## Normative specifications

| Version | Document | Adds |
|---:|---|---|
| 1 | [isa-v1.md](../isa-v1.md) | Integer ALU, LW/SW, BEQ/BNE, JAL, ECALL, HALT |
| 2 | [isa-v2.md](../isa-v2.md) | JALR |
| 3 | [isa-v3.md](../isa-v3.md) | CMP (six conditions) |
| 4 | [isa-v4.md](../isa-v4.md) | Privilege, CSR, ERET, SFENCE, traps/IRQs |

Related: [mmu-v1.md](../mmu-v1.md), [tlb-v1.md](../tlb-v1.md), [exceptions-interrupts.md](../exceptions-interrupts.md), [platform.md](../platform.md).

There is **no** division, multiply, shift, or floating-point opcode. Do not invent instructions beyond these documents.

## Opcode summary (`op` bits 31:26)

| op | Mnemonic | Min ISA | Notes |
|---:|---|---:|---|
| 0x00 | NOP | 1 | reserved bits must be zero |
| 0x01 | HALT | 1 | host stop after retire |
| 0x02 | ADD | 1 | modulo 2^32 |
| 0x03 | SUB | 1 | modulo 2^32 |
| 0x04 | AND | 1 | |
| 0x05 | OR | 1 | |
| 0x06 | XOR | 1 | |
| 0x07 | ADDI | 1 | `imm18` sign-extended |
| 0x08 | LW | 1 | aligned word load |
| 0x09 | SW | 1 | aligned word store |
| 0x0a | BEQ | 1 | PC-relative word offset |
| 0x0b | BNE | 1 | PC-relative word offset |
| 0x0c | JAL | 1 | link in `rd` |
| 0x0d | ECALL | 1 | cause 3 |
| 0x0e | JALR | 2 | |
| 0x0f | CMP | 3 | conditions 0..5 |
| 0x10 | CSR | 4 | supervisor only |
| 0x11 | ERET | 4 | supervisor only |
| 0x12 | SFENCE | 4 | supervisor TLB flush |

Opcodes `0x13`–`0x3f` are illegal at every ISA version.

## Emulator module map

| Module | Headers / sources | Role |
|---|---|---|
| `cpu_state` | `cpu_state.h`, `cpu_state.c` | PC, privilege, halt, ISA version, fault diagnostics |
| `register_file` | `register_file.h`, `register_file.c` | `r0`–`r15` with `r0` hardwired zero |
| `memory_interface` | `memory_interface.h`, `memory_interface.c` | Fetch/load/store via MMU + RAM/MMIO |
| `instruction_decoder` | `instruction_decoder.h`, `instruction_decoder.c` | Encoding validation and field extract |
| `instruction_executor` | `instruction_executor.h`, `instruction_executor.c` | Per-opcode semantics |
| `exception_handler` | `exception_handler.h`, `trap.c` | Sync traps, CSR/ERET/SFENCE, IRQ delivery |
| `instruction_trace` | `instruction_trace.h`, `instruction_trace.c` | Optional single-step history ring |

`haeun_vm` is the public aggregate. `haeun_vm_step` is the single-step entry: fetch → decode → execute → optional trace.

## Correctness rules

1. Only opcodes defined for the selected ISA version retire.
2. Register indices are four-bit fields (`0..15`); `r0` writes are discarded after each retired instruction.
3. Reserved encoding bits must be zero or the instruction is illegal (cause 0).
4. Arithmetic is modulo 2^32; CMP uses signed two's-complement relational rules.
5. LW/SW require 4-byte aligned virtual addresses.
6. CSR/ERET/SFENCE require supervisor mode (cause 4 otherwise).
7. Execution is deterministic for a fixed guest image, feeds, and step budget.

## Verification map

| Concern | Primary tests |
|---|---|
| Every opcode smoke + unused ops | `tests/opcode_matrix_tests.c` |
| ALU / branch / load-store / UART faults | `tests/vm_tests.c` |
| CMP operand matrix | `tests/vm_tests.c` |
| Privilege, ERET, timer IRQ | `tests/isa4_tests.c` |
| SFENCE / TLB | `tests/tlb_tests.c` |
| Decoder / RF / trace modules | `tests/cpu_module_tests.c` |
