# Process management v1

Status: **guest PCB, RR + timer preemption, privilege transitions, and the listed syscalls are implemented and tested**. Depends on [isa-v4.md](isa-v4.md), [mmu-v1.md](mmu-v1.md), and [platform.md](platform.md).

## Goal

Run multiple isolated user processes with kernel/user stacks, full GPR context switches, Round-Robin scheduling, and timer preemption on IRQC source 0. Switching is performed by the guest trap path on the emulated HAEUN-32 CPU—not by host-side function alternation.

## Process states

| Value | Name | Meaning |
|---:|---|---|
| 0 | `FREE` | Slot unused; resources reclaimable |
| 1 | `READY` | Runnable |
| 2 | `RUNNING` | Selected on the CPU |
| 3 | `SLEEP` | Blocked until `sleep_deadline` |
| 4 | `WAIT` | Blocked in `WAIT` for a child |
| 5 | `ZOMBIE` | Exited; waits for parent `WAIT` to reclaim |

## PCB (256-byte stride, base `0x0000A000`)

| Offset | Field |
|---:|---|
| 0 | `state` |
| 4 | `pid` |
| 8 | `ppid` |
| 12 | `epc` |
| 16 | `exit_code` |
| 20 | `sleep_deadline` (timer `COUNT_LO`; 0 if unused) |
| 24 | `wait_pid` (0 = any child) |
| 28 | `ptbr` word for this address space |
| 32 | `kstack_top` |
| 36 | `ustack_top` |
| 40 | `ustack_base` |
| 44–56 | `fd[0..3]` (0 = closed; 1 = console) |
| 64–120 | saved `r1`…`r15` |

`NPROC = 4`. Current index word at `0x0000A400`. Next pid counter at `0x0000A404`.

## Stacks and isolation

- Each live process has a dedicated **user stack** page and a dedicated **kernel stack** page.
- Trap entry switches `r13` to `kstack_top` after parking the user `r13` in the PCB, then saves GPRs.
- Each process has its own L1/L0 tables: kernel pages lack `U`; user code/stack pages set `U`. Context switch writes `ptbr` and executes `SFENCE`.
- User mode cannot execute CSR/`ERET`/`SFENCE` (ISA v4 privilege fault).

## Scheduler

Round-Robin over `READY`/`RUNNING` processes. Timer quantum is 32 ticks (one-shot compare). On cause `16`: acknowledge IRQC, wake `SLEEP` deadlines, pick the next runnable process, re-arm the timer immediately before `ERET`.

## Syscall ABI

`ECALL` cause `3`. `r1` = number; `r2`–`r7` = args; return in `r1`. Advance `epc` by 4 for synchronous returns. Invalid args or unknown numbers return `(uint32_t)-1` without corrupting other process state.

| # | Name | Args | Effect |
|---:|---|---|---|
| 0 | `EXIT` | `r2` code | Become `ZOMBIE`; wake waiting parent; schedule |
| 1 | `WRITE` | `r2` fd, `r3` byte | Console fd only; UART TX |
| 2 | `READ` | `r2` fd | Console: UART pop or `-1` if empty |
| 3 | `OPEN` | `r2` device id (`1`=console) | Allocate fd slot |
| 4 | `CLOSE` | `r2` fd | Clear fd slot |
| 5 | `FORK` | — | Copy PCB/context; child `r1=0`, parent `r1=child_pid` |
| 6 | `EXEC` | `r2` entry VA | Replace `epc`, reset user stack pointers |
| 7 | `WAIT` | — | Reap a `ZOMBIE` child or block in `WAIT` |
| 8 | `SLEEP` | `r2` ticks | Block until `COUNT_LO + ticks` |
| 9 | `GETPID` | — | Return `pid` |
| 10 | `YIELD` | — | Cooperative RR |

## Resource recovery

`WAIT` moves a reaped child from `ZOMBIE` to `FREE` and clears its fd table. `EXIT` does not free the slot until reaped (zombie semantics).

## Tests

Guest image runs init + forked CPU-bound and I/O-bound (`SLEEP`+`WRITE`) children under real timer interrupts; UART shows interleaved output and distinct PIDs.
