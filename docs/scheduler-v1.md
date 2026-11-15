# Scheduler v1 (cooperative + timer preemption)

Status: **cooperative and timer-preemptive switching are implemented and tested**. Depends on [kernel-v1.md](kernel-v1.md), [isa-v4.md](isa-v4.md), and [platform.md](platform.md) timer/IRQC.

## Goal

Run two user tasks with per-task stacks. Switch on `SYS_YIELD`, `SYS_EXIT`, or a timer interrupt (IRQC source 0 / cause `16`).

## Syscalls

| Number | Name | Effect |
|---:|---|---|
| 0 | `SYS_EXIT` | Mark the current task dead; schedule the next runnable task. If none remain, `HALT` with `r1` = exit code (`r2`). |
| 1 | `SYS_PUTC` | UART putc; returns to the same task. |
| 2 | `SYS_YIELD` | Save the current task, advance `epc` past `ECALL`, switch to the next alive task. |

## Task control block (guest physical)

128-byte stride:

| Offset | Field |
|---:|---|
| 0 | `alive` (1 = runnable, 0 = dead) |
| 4 | `epc` |
| 8–64 | saved `r1`…`r15` |

- Task 0 base `0x00005000`, task 1 base `0x00005080`
- Current index at `0x00005100`

Stacks: task 0 top `0x00104000`, task 1 top `0x00105000`.

## Timer preemption

At boot the kernel maps IRQC (`0x10000000`) and timer (`0x10001000`), enables IRQC source 0, programs a one-shot compare `COUNT + quantum` (quantum `32` ticks), and enters user mode with `status.IE=1`.

On cause `16`:

1. Save GPRs/`epc` (interrupt `epc` is already the next user PC; do not add 4).
2. Write `COMPLETE=0`, pick the next alive task.
3. Re-arm compare to `COUNT_LO + quantum` and set `CONTROL=1` immediately before `ERET` (so the handler itself does not consume the new quantum).
4. `ERET` (restoring `IE` via `PIE`).

## Switch policy

Round-robin over alive tasks. Trap entry saves GPRs into the current TCB, then branches on `cause` (timer) or syscall number.

## Incremental delivery

1. Cooperative two-task yield/exit.
2. Timer-preemptive switching using IRQC source 0 (this document).
