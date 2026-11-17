# R3.1 Pre-Implementation Audit

Host-side policy-state validation only. Kernel sources are not modified in R3.1.

## Baseline kernel hashes (pre-R3.1)

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `dee3a76622a8d2acfcb5a0e958b14f74056dbb52` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `8349bdacf7019e239941fc4a6f39c40088b637c4` |

## Verified architecture

| Item | Actual |
|---|---|
| RR ID | `0` |
| Preemptive FIFO ID | `1` (`SCHED_POLICY_FCFS`) |
| Policy word | `0xA474` |
| Boot init | `sched_pol_init` copies `sched_pol_boot` → `0xA474` |
| `ready_since` | PCB+160; last READY entry (`telem_ready` / wake / child) |
| Eligibility | READY(1) or RUNNING(2); slots 0..2 |
| FIFO order | signed modular older `ready_since`; tie → lower slot |
| Commit | core `pick_next` only |
| Slot 3 | no AS; never selected |

## Safety constraints for R3.1

1. Validator must not write guest RAM or CSRs.
2. Validator must not execute guest instructions.
3. Validator must not change `0xA474`.
4. Signed modular FIFO ordering is only reliable when pairwise separations are strictly less than `2^31`; exact `2^31` is ambiguous.
5. Lifetime bound that READY wait stays `< 2^31` is an **external assumption** — a snapshot can verify pairwise comparability, not the lifetime bound.
6. FREE / slot3 telemetry must not be treated as live candidates.
7. Compatibility PASS means `transition_preconditions_satisfied` for implemented checks only — not proof of a safe R3.2 transition.

## Observation channels

- Guest RAM via existing `haeun_telem_read` + direct const reads of PCB fields (ppid, epc, ptbr).
- Host VM fields: `timer_count`, `timer_compare`, `timer_control`, `csr_ptbr`, `pc` (host view; not guest-stepped during validate).

## Decision

Proceed with host-only modules under `pval_*`. No kernel edits.
