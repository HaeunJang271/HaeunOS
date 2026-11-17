# R3.2 Pre-Implementation Audit

## Baseline kernel hashes (pre-R3.2 / post-R3.1)

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `dee3a76622a8d2acfcb5a0e958b14f74056dbb52` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `8349bdacf7019e239941fc4a6f39c40088b637c4` |

## Execution model

`haeun_vm_step` is a single-threaded host interpreter step. No guest concurrency.
Timer traps run **inside** a step. Between steps, no guest instruction executes.

**Safe point:** host API invoked only between steps (not mid-`haeun_vm_step`).
Serialized policy word write at `0xA474` is then race-free w.r.t. `policy_select`.

This is **serialized commit**, not hardware atomicity / SMP safety.

## Policy IDs

- RR = 0, Preemptive FIFO = 1 at `0xA474`
- Boot: `sched_pol_init` copies `sched_pol_boot`
- Default remains RR

## R3.1 integration constraint (blocker for many RR→FIFO)

Mandatory preconditions treat **UNKNOWN as reject**.
R3.1 cannot prove lifetime `ready_since` span `< 2^31` from a snapshot.
Therefore:

- **RR → FIFO** with eligible READY/RUNNING: typically **REJECTED** (timestamp UNKNOWN).
- **RR → FIFO** with zero eligible candidates: timestamp N/A → may PASS vacuous ordering.
- **FIFO → RR**: timestamp not required; may PASS when invariants allow.

No new PCB sequence field in R3.2 (would be ABI/layout change). Do not weaken R3.1.

## Transition mechanism

Host writes only the policy word in guest RAM after prepare+recheck.
Kernel selection code unchanged. Not a boot-time image patch presented as runtime.

## Decision

Proceed with host-side transaction protocol; keep RR→FIFO rejected when R3.1 cannot PASS timestamp gates.
