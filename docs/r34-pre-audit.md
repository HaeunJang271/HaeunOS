# R3.4 Pre-Implementation Audit

## Baseline hashes

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `dee3a76622a8d2acfcb5a0e958b14f74056dbb52` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `8349bdacf7019e239941fc4a6f39c40088b637c4` |

## 1. Active scheduler path

Timer/syscall → trap → demote RUNNING→READY (`telem_leave_run` + `telem_ready`) → `policy_select` → `telem_select` → RUNNING → `set_curr` / `switch_as`. All inside guest, driven by host `haeun_vm_step`.

## 2. Safe point

Between `haeun_vm_step` calls (single-threaded interpreter). R3.2 commits policy word here. Not hardware atomicity / SMP.

## 3–4. Transaction / rollback

R3.2: request→prepare→commit; rollback restores **policy ID only**.

## 5. R3.1 on active state

Yes: snapshot/invariants/timestamps/compat are host read-only over live RAM. Active READY/RUNNING is visible.

## 6. Why RR→FIFO → UNKNOWN

FIFO uses 32-bit modular `ready_since`. Snapshot cannot prove lifetime span `<2^31` → UNKNOWN with eligible candidates. Vacuous empty set may PASS (not active).

## 7. `ready_since` updates

Set from timer `COUNT_LO` in `telem_ready`, `telem_wsleep`, `telem_wwait`, `telem_child`, host spawn. Updated on every READY entry including timer demotion.

## 8. Unused fields

PCB `+172..+255` reserved (no kernel writers found). Globals `0xA46C`/`0xA470` reserved and unused in code.

## 9. Timestamp width

32-bit `COUNT_LO`; modular compare; wrap every `2^32` ticks.

## 10. Ordering representation without ABI break

**Host-only insufficient:** selection is guest `policy_fcfs`. Need kernel metadata.

**Candidate (chosen for R3.4 implementation):** 64-bit mono READY sequence:

| Location | Field |
|---|---|
| `0xA46C` / `0xA470` | global `ready_seq` lo/hi (repurpose unused reserved) |
| PCB `+172` / `+176` | per-process `ready_seq` lo/hi |

Assign on every READY entry. FIFO compares sequences (unsigned 64-bit; tie → lower slot). Keep `ready_since` for telemetry.

**Bound:** safety claimed only while global wrap count remains 0 (`2^64` READY entries since boot). Finite counter; wrap → UNKNOWN. Not mathematically unbounded.

## Decision

Proceed with kernel READY-sequence + R3.1 seq validation + active mid-run transactions + host scheduler traces. Do not weaken UNKNOWN→PASS.
