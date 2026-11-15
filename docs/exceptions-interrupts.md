# Exception and interrupt semantics

This document is the architectural contract for synchronous faults, environment calls, and asynchronous interrupts. It separates **implemented v1–v3 VM stop behavior** from the **ISA v4 privilege and interrupt delivery model** in [isa-v4.md](isa-v4.md). Timer/IRQC MMIO may latch pending bits without CPU vectoring until delivery is wired.

## Implemented synchronous stops (ISA v1–v3)

The C17 VM retires or rejects one instruction at a time. On a synchronous fault it leaves `PC` at the faulting instruction (except where an instruction explicitly advances `PC` before stopping), records `fault_pc` and a numeric `cause`, and returns a stop result to the host. There is **no guest trap vector and no handler** in the current ISA.

| Cause | Meaning | Typical VM result | PC after stop |
|---:|---|---|---|
| 0 | Illegal opcode or reserved bits nonzero | `HAEUN_VM_ILLEGAL_INSTRUCTION` | Faulting instruction |
| 1 | Address out of range / unmapped access | `HAEUN_VM_MEMORY_FAULT` | Faulting instruction |
| 2 | Alignment fault (instruction or word data) | `HAEUN_VM_ALIGNMENT_FAULT` | Faulting instruction |
| 3 | `ECALL` environment call | `HAEUN_VM_TRAP` | Faulting instruction (`PC` unchanged) |

Additional stop results that are not architectural guest causes:

- `HAEUN_VM_HALTED`: `HALT` retired; `PC` advanced by 4; further steps remain halted.
- `HAEUN_VM_STEP_LIMIT`: host instruction budget exhausted without an architectural fault.
- `HAEUN_VM_BAD_ARGUMENT`: host API misuse; guest state is not defined as changed.

Control-transfer destinations (branch, JAL, JALR) are validated on the **next fetch**. An out-of-range or misaligned target becomes the new `PC` and then faults on fetch with the corresponding cause. `r0` write suppression does not create a fault.

`ECALL` is the only intentional synchronous trap in v1–v3. Without a privilege mode or trap return instruction, it is a defined stop for host/runtime mediation (for example compiler stack-overflow checks), not a recoverable kernel syscall path.

## Privilege modes and interrupt delivery (ISA v4)

Privilege modes, CSRs, `CSRRW`/`CSRRS`/`CSRRC`, `ERET`, trap entry, and between-instruction interrupt sampling are specified in [isa-v4.md](isa-v4.md). Board sources remain in [platform.md](platform.md).

Current implementation status:

- Timer and IRQC MMIO pending/enable/claim/complete are implemented and tested.
- ISA v4 CSR/`ERET`, privilege modes, synchronous guest trap entry, and IRQC interrupt vectoring are implemented and tested.
- MMU v1 `ptbr` and two-level walks deliver page faults (cause 5) under ISA v4; v1–v3 guests remain bare (`VA=PA`) and still stop on faults without a handler.

## Cause namespace reservation

To keep host diagnostics and a future guest-visible cause CSR compatible:

| Range | Use |
|---|---|
| 0–3 | Frozen synchronous causes used by the current VM |
| 4 | Privilege violation (ISA v4) |
| 5 | Page fault (MMU v1) |
| 6–15 | Reserved for future synchronous exceptions |

| 16–31 | Reserved for interrupt causes (16 + source id initially) |
| ≥32 | Reserved; must not be emitted by current software |

Changing the meaning of causes 0–3 requires a new ISA major version.

## Verification requirements

- Every implemented opcode has illegal-encoding and success paths tested.
- Memory and alignment faults leave `PC` and `cause` as specified.
- `ECALL` returns `HAEUN_VM_TRAP` with cause 3 and does not invent a handler.
- Device PENDING/ENABLE/CLAIM/COMPLETE may be tested as MMIO without claiming CPU interrupt **delivery**.
- TLB v1 is present; PTE edits require a `ptbr` rewrite or supervisor `SFENCE` to flush.
