# HAEUN-32 ISA version 4 privilege and traps

ISA v4 preserves every v1–v3 instruction encoding. It adds two privilege modes, a small supervisor CSR file, CSR access instructions, and a trap return instruction so the board IRQC and timer can deliver recoverable interrupts. Images declare ISA version 4 in HXF. A v1–v3 VM must treat the new opcodes as illegal.

## Privilege modes

| Mode | Code | Intent |
|---|---:|---|
| User | 0 | Unprivileged programs |
| Supervisor | 1 | Kernel, firmware after handoff, and trap handlers |

Reset enters supervisor mode with global interrupts disabled (`status.IE=0`). Mode changes only through trap entry and `ERET`. Host code must not invent privilege.

## Supervisor CSRs

CSR numbers are 12-bit. User-mode CSR access raises a synchronous privilege fault (cause 4) without retiring the instruction. Supervisor mode may read and write the registers below. Other CSR numbers are illegal (cause 0).

| CSR | Number | Fields |
|---|---:|---|
| `status` | `0x000` | bit 0 `IE` (global interrupt enable); bit 1 `PIE` (previous `IE`); bits 3:2 `PP` (previous privilege, 0 or 1); other bits read as zero and ignore writes |
| `trapvec` | `0x001` | trap entry PC; must be 4-byte aligned; misaligned write is illegal and leaves the CSR unchanged |
| `epc` | `0x002` | PC saved on trap entry |
| `cause` | `0x003` | cause code written on trap entry |
| `tval` | `0x004` | trap-specific value (faulting address when defined, else 0) |
| `scratch` | `0x005` | supervisor scratch |
| `ptbr` | `0x006` | bit 0 paging MODE; bits 31:12 root table PPN; bits 11:1 reserved zero (see [mmu-v1.md](mmu-v1.md)) |

## New instructions

### CSR (`op=0x10`)

I-format fields: `rd`, `ra`, and `imm18`. Bits `imm18[11:0]` select the CSR. Bits `imm18[13:12]` select the operation; bits `imm18[17:14]` must be zero.

| `imm18[13:12]` | Mnemonic | Effect |
|---:|---|---|
| 1 | `CSRRW` | `rd = CSR`; `CSR = ra` (read old value first) |
| 2 | `CSRRS` | `rd = CSR`; `CSR = CSR \| ra` |
| 3 | `CSRRC` | `rd = CSR`; `CSR = CSR & ~ra` |
| 0 | — | illegal |

`r0` write suppression applies to `rd`. When `rd` is `r0`, the CSR is still read for side-effect-free observation except that implementations may skip the architectural read data path; the CSR write still occurs. When `ra` is `r0` for `CSRRS`/`CSRRC`, the CSR is read and not modified.

### ERET (`op=0x11`)

All non-opcode bits must be zero. Illegal in user mode (cause 4). In supervisor mode: `PC = epc`, privilege = `status.PP`, `status.IE = status.PIE`, then `status.PIE = 0` and `status.PP = 0`. No other registers change.

### SFENCE (`op=0x12`)

All non-opcode bits must be zero. Illegal in user mode (cause 4). In supervisor mode: flush the entire TLB (see [tlb-v1.md](tlb-v1.md)), then advance `PC` by 4. Does not modify CSRs or general registers. Assembler mnemonic: `sfence`.

## Trap entry

Used for synchronous faults under ISA v4 and for asynchronous interrupt delivery:

1. `epc` ← faulting PC (synchronous) or the PC of the next instruction that would have run (interrupt).
2. `cause` ← architectural cause; `tval` ← defined auxiliary value or 0.
3. `status.PIE ← status.IE`; `status.IE ← 0`; `status.PP ←` current mode; mode ← supervisor.
4. `PC ← trapvec`.

Nested interrupts are forbidden until software sets `status.IE` again. `HALT` remains a host-visible stop and does not enter the trap path.

## Cause codes

Causes 0–3 keep their v1–v3 meanings. ISA v4 adds:

| Cause | Meaning |
|---:|---|
| 4 | Privilege violation (CSR/`ERET` in user mode) |
| 5 | Page fault (invalid PTE, permissions, or user/`U` mismatch) |
| 16 + source | Interrupt from IRQC source id (`0` timer, `1` UART RX, `2` block) |

## Interrupt sampling (delivery)

Between retired instructions, when `status.IE=1`, the VM inspects IRQC: the lowest source id that is both pending and enabled is delivered with cause `16 + id`. Device PENDING/ENABLE semantics remain in [platform.md](platform.md). The VM implements this sampling after each retired instruction (including `ERET`).

## Toolchain and containers

HOBJ gains a new object version that records ISA 4 when the assembler emits CSR/`ERET`. HXF remains format v1 and carries `isa_version=4`. Until that object version exists, assembly tests may pack instruction words directly.

## Incremental delivery

1. Ratify this document; implement timer and IRQC MMIO without CPU delivery.
2. Implement CSR/`ERET`, privilege state, and synchronous privilege faults.
3. Wire between-instruction interrupt delivery from IRQC into the trap path (done).
4. Supervisor `SFENCE` TLB flush (done).
