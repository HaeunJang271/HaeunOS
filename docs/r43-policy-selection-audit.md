# R4.3 Policy Selection Audit

## Implementation (source)

| Policy | Rule (guest) | Host mirror |
|--------|--------------|-------------|
| RR (`policy_rr`) | Scan from `curr+1` wrap; READY(1) or RUNNING(2) | `haeun_policy_select_host` |
| Preemptive FIFO (`policy_fcfs`) | Min unsigned 64-bit `ready_seq`; tie → lower slot | same |

Files: `kernel/process/proc_policy.s`, `src/policy_verify.c`.

## Fresh run (2026-11-18) — `r43_active_verification_tests`

| Phase | Apply | Selection events | Mirror matches | Note |
|-------|-------|------------------|----------------|------|
| RR→FIFO | step 22371 → pol=1 | 61 | 61 | All recorded events are **post-apply under FIFO** |
| FIFO→RR | step 22371 → pol=0 | 61 | 61 | All recorded events are **post-apply under RR** |

**Behavioral proof:** expected slot from host mirror equals observed `curr` after each switch.  
Policy ID alone is insufficient; this compares selection outcomes.

Pre-apply selection events were not observed in these traces (no `curr` change before apply). Post-apply evidence still covers both policies’ rules.

Artifact: `research/experiments/r43-selection-trace.json`.
