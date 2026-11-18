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
| 64–120 | saved `r1`…`r15` (fork copies 64–127; offset 112 is unused but copied) |
| 128–168 | R1.2 observational telemetry |
| 172–176 | R3.4 `ready_seq` lo/hi (FIFO ordering; scheduling input for policy 1) |
| 180–255 | reserved |

`NPROC = 4`. Current index word at `0x0000A400`. Next pid counter at `0x0000A404`. Scratch `0xA410`–`0xA44B`. Fault diag `0xA450`–`0xA45F`. `IDLE_FLAG` `0xA460`.

### Observational telemetry (R1.2)

Counters are unsigned 32-bit and wrap modulo `2^32`. Timestamps use timer `COUNT_LO` with unsigned modular subtraction. An individual unaccounted interval must be shorter than `2^32` ticks. Telemetry never feeds `pick_next`.

Per-PCB (base + stride):

| Offset | Field | Meaning |
|---:|---|---|
| +128 | `run_ticks` | `COUNT_LO` spent in `RUNNING` |
| +132 | `ready_wait_ticks` | time in `READY` before selection |
| +136 | `sleep_ticks` | `SLEEP` intervals until wake |
| +140 | `blocked_ticks` | `WAIT` intervals until wake |
| +144 | `dispatch_count` | successful `pick_next` selections (includes self) |
| +148 | `complete_lo` | first `ZOMBIE` timestamp |
| +152 | `complete_valid` | 1 after `complete_lo` written |
| +156 | `account_lo` | `COUNT_LO` at last dispatch / boot |
| +160 | `ready_since` | `COUNT_LO` when entered `READY` |
| +164 | `sleep_since` | `COUNT_LO` when entered `SLEEP` |
| +168 | `blocked_since` | `COUNT_LO` when entered `WAIT` |

Globals (same PCB page; do not use `0xA460`):

| Address | Field |
|---|---|
| `0xA464` | `timer_irqs` — guest timer-trap entries (`do_timer` / `idle_trap`) |
| `0xA468` | `context_switches` — selections where PCB index changed |
| `0xA46C` | R3.4 global `ready_seq` lo |
| `0xA470` | R3.4 global `ready_seq` hi |
| `0xA478` | R3.4 `ready_seq` wrap sticky |

Accounting points: leave `RUNNING` (timer demote, `YIELD`, `SLEEP`, `WAIT`, `EXIT`, fault) adds into `run_ticks` from `account_lo`. Kernel handler time while the PCB is still `RUNNING` is included; time after the state leaves `RUNNING` is not. `dispatch_count` increments on every successful pick; `context_switches` only when the selected index differs from the previous current index. Forked children zero `+128`–`+168` and set `ready_since`; parents are not cleared. Host `APP_SPAWN` memset clears the slot and writes `ready_since` from host `timer_count`.

## Stacks and isolation

- Each live process has a dedicated **user stack** page and a dedicated **kernel stack** page.
- Trap entry switches `r13` to `kstack_top` after parking the user `r13` in the PCB, then saves GPRs.
- Each process has its own L1/L0 tables: kernel pages lack `U`; user code/stack pages set `U`. Context switch writes `ptbr` and executes `SFENCE`.
- User mode cannot execute CSR/`ERET`/`SFENCE` (ISA v4 privilege fault).

## Scheduler

Round-Robin over `READY`/`RUNNING` processes in slots 0..2 (`sched_slots` = 3). R2.0 splits RR candidate selection (`policy_rr` / `policy_select`, ID `0` at `0xA474`) from scheduler commit in `pick_next`; RR semantics are unchanged (see `docs/r20-scheduler-policy.md`). The PCB table still has 4 rows; slot 3 has no address space and is never selected. User timer quantum is 32 ticks. `restore_ret` re-arms compare as `timer_count + 96` and acknowledges a stale timer IRQ so the handler tail cannot consume the one-shot or steal `ERET`. `arm_timer` writes both compare halves, including the carry out of the low word. On cause `16`: acknowledge IRQC, wake `SLEEP` deadlines, pick the next runnable process, re-arm immediately before `ERET`.

A failed pick does not `ERET` into a `SLEEP`, `WAIT`, `ZOMBIE`, or `FREE` PCB. `resume_or_idle` / `idle_enter` set `IDLE_FLAG` at `0xA460` and wait in supervisor with timer IRQs enabled. `trap_entry` sees the flag and does not rewrite the saved user PCB. Wake condition is signed(`COUNT_LO - deadline`) >= 0, which is the unsigned deadline test for sleeps shorter than 2^31 ticks. If nothing is `SLEEP` and nothing is runnable, the kernel halts: `r1` is the current exit code when that PCB is `ZOMBIE`, otherwise `r1 = 4` (WAIT deadlock).

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
| 52 | `TRYWAIT` | — | Non-blocking: reap one own `ZOMBIE` or return `-1` (M24.3 init idle) |

## Resource recovery (dual-stage)

Host `reap_pid` (ECALL mux / fault) destroys process-owned windows, evt queues, IPC, services, FDs, sockets, and scrubs AS **before** the guest PCB becomes or remains `ZOMBIE`.

Guest `EXIT`/`fault_exit` mark `ZOMBIE`, call `orphan_reparent` (children’s `ppid` → PCB0 init pid), and `wake_wait`. `WAIT` moves a reaped child from `ZOMBIE` to `FREE` and clears its fd table.

Desktop idle calls `TRYWAIT` then `YIELD` so adopted/exited children become `FREE` without blocking. `APP_SPAWN` orphan memset remains a fallback for unused-slot `ZOMBIE` PCBs.

Idempotent guest `EXIT`: if already `ZOMBIE`, only reschedule (mirrors `fault_exit`).

## Tests

Guest image runs init + forked CPU-bound and I/O-bound (`SLEEP`+`WRITE`) children under real timer interrupts; UART shows interleaved output and distinct PIDs.
