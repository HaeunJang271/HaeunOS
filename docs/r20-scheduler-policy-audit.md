# R2.0 Source Audit — Scheduler Policy Abstraction

Audit only. No files were modified for this document. Source is authoritative.

## 1. Active kernel

Process image sources (`tools/build_process_image.py` `KERNEL_SRCS`):

- `kernel/process/proc_boot.s`
- `kernel/process/proc_trap.s`
- `kernel/process/proc_sys.s`
- `kernel/process/proc_fork.s`
- `kernel/process/proc_fork_mem.s`
- `kernel/process/proc_orphan.s`
- `kernel/process/proc_sched.s`
- `kernel/process/proc_telem.s`

`kernel/scheduler/` is a separate demo image and is **not** linked into process-v1.

## 2. Call graph (process-v1)

```text
trap_entry
  ├─ cause 16 → do_timer
  │                 wake_all → pick_next → [idle_enter | restore_ret]
  ├─ cause 3  → do_syscall → sys_* 
  │                 yield/sleep/exit/fault/wait_block → resume_or_idle
  │                     pick_next → [idle_enter | restore_ret]
  └─ IDLE_FLAG → idle_trap
                    wake_all → pick_next → [it_wait/idle_dead | restore_ret]

idle_enter loop: wake_all → pick_next → [ie_go/restore_ret | ie_spin | idle_dead]
```

`pick_next` callers: `do_timer`, `resume_or_idle`, `idle_enter`, `idle_trap`.

## 3. Current `pick_next` semantics (`proc_sched.s`)

1. `cur_pcb` → `r8`.
2. If state `RUNNING` (2): `telem_leave_run`, store `READY` (1), `telem_ready`.
3. Load `curr` from `0xA400` into `r6`; save old index in `r13`.
4. `sched_slots` → 3 (slots `0..2` only; slot 3 excluded).
5. Scan `curr+1` with wrap at 3.
6. Accept state `READY` (1) or `RUNNING` (2).
7. On hit: `telem_select`, store `RUNNING`, `set_curr`, `switch_as`, return `r1=1` / `r8=PCB`.
8. On miss: `cur_pcb`, return `r1=0`.

Policy (scan order / eligibility) is interleaved with core mutations (demote, telem, `set_curr`, `switch_as`).

## 4. Related mechanics

| Mechanism | Location | Notes |
|---|---|---|
| `set_curr` | `proc_sched.s` | PCB `r8` → index at `0xA400` |
| `switch_as` | `proc_sched.s` | CSR PTBR from PCB+28, `sfence` |
| `restore_ret` | `proc_trap.s` | `arm_timer` (+96), `ack_timer`, GPR/EPC restore, `eret` |
| Idle | `IDLE_FLAG` `0xA460` | Unchanged R1.1 behavior |
| Telemetry | `proc_telem.s` | R1.2 counters; host R1.3 inspector reads only |

## 5. RR scan order (invariant for R2.0)

For `curr = c` and `SCHED_SLOTS = 3`, examine slots:

`(c+1) mod 3`, `(c+2) mod 3`, `(c+3) mod 3` (= `c`).

First slot with state READY or RUNNING wins. After demotion, the former current is READY and may be re-selected (self-dispatch).

## 6. Tests that encode RR/telemetry expectations

- R1.1: `tests/r11_sched_tests.c`, `tests/r11_timer_tests.c`
- R1.2: `tests/r12_telem_tests.c`
- R1.3: `tests/r13_telem_inspect_tests.c`

## 7. R2.0 separation intent (post-audit)

| Layer | May do | Must not do |
|---|---|---|
| Policy (`policy_rr`) | Return candidate slot or none | Mutate PCB/`curr`/PTBR/timer/telem |
| Core (`pick_next`) | Demote, validate, telem, `set_curr`, `switch_as`, idle handoff | Change RR scan rules |

Policy ID: `SCHED_POLICY_RR = 0` only. No function pointers. No runtime switching.
