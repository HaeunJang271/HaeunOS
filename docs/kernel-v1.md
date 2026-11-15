# Minimal HAEUN kernel v1

Status: **syscall trap, host page-map helpers, guest identity map, user-mode return, and timer-preemptive scheduling are implemented and tested**. Depends on [isa-v4.md](isa-v4.md), [mmu-v1.md](mmu-v1.md), [scheduler-v1.md](scheduler-v1.md), and [software-abi.md](software-abi.md).

## Goal

Provide a guest kernel that services `ECALL` syscalls, installs an identity map, enables `ptbr`, and returns to a user page that exercises `SYS_PUTC` / `SYS_EXIT`.

## Syscall ABI v1

`ECALL` enters the supervisor trap path with cause `3`. Argument and return registers follow the provisional calling convention:

| Register | Role |
|---|---|
| `r1` | syscall number on entry; return value on `ERET` |
| `r2`–`r7` | arguments |
| other GPRs | preserved by the v1 handler except scratch used inside the handler |

| Number | Name | Arguments | Effect |
|---:|---|---|---|
| 0 | `SYS_EXIT` | `r2` = exit code | Ends the current task; see [scheduler-v1.md](scheduler-v1.md) |
| 1 | `SYS_PUTC` | `r2` = character | writes low byte to UART DATA; returns `0` in `r1` and `ERET`s |
| 2 | `SYS_YIELD` | — | Cooperative reschedule; see [scheduler-v1.md](scheduler-v1.md) |

Unknown numbers return `(uint32_t)-1` in `r1` and `ERET`. Because synchronous traps save the faulting `ECALL` PC in `epc`, returning handlers must add 4 to `epc` before `ERET`. Interrupts remain disabled across the handler until software sets `status.IE`.

## Page-map helpers (host)

`include/haeun/pt.h` builds guest-physical page tables in caller-owned RAM using the [mmu-v1.md](mmu-v1.md) PTE layout.

## Guest boot sequence (`kernel/scheduler/entry.s`)

1. `trap_entry` is linked at VA `0x00100000` and installed into `trapvec`.
2. `_start` writes L1/L0 leaves for:
   - `0x00100000` kernel RX (no `U`)
   - `0x00101000` user RXU
   - `0x10000000` IRQC RW, `0x10001000` timer RW, `0x10002000` UART RW
3. Enables `ptbr` (`MODE=1`, root at PA `0x2000`), enables IRQC source 0, arms the one-shot timer (quantum `32`), sets `status.PIE=1` / `PP=user`, and `ERET`s to task 0.
4. Two user tasks print via `SYS_PUTC`; the timer path preempts without `SYS_YIELD` (see [scheduler-v1.md](scheduler-v1.md)).

## Incremental delivery

1. Syscall trap + `SYS_EXIT`/`SYS_PUTC` + host page-map helpers.
2. Guest identity map enable through `ptbr` and a user-mode return.
3. Cooperative scheduler with per-task stacks (`SYS_YIELD`; see [scheduler-v1.md](scheduler-v1.md)).
4. Timer-preemptive switching on IRQC source 0 (done).
