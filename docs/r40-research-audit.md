# R4.0 Research Audit

Date: 2026-11-18. Method: source/Makefile/test inventory against milestone claims.  
Kernel SHA-256 (unchanged vs R3.7 baseline):

| File | SHA-256 |
|------|---------|
| `proc_sched.s` | `f378293d24ef74fe9af42615959e40c5c1de038d49a38590adfa562bf200d7d7` |
| `proc_policy.s` | `38a79c49d2f9f9d72589f2c13e63f96c1b279bb051e02ac8bb1328f7d81299de` |
| `proc_trap.s` | `e6cb2a9823a5e6a4d3bcba1322fcaad4e4732fb3138972df7cef672ebe3bc020` |
| `proc_telem.s` | `2d7dbeacdb4326838487a580f946de660a39c141c408e6767d529a89c06a705a` |
| `proc_fork.s` | `d895e560f52a33610fb9f63271d854bb38d7941422bc9a4eb093f57ed60ca603` |

## A. Milestone capabilities (summary)

| Milestone | Kernel | Host | Tests | Status |
|-----------|--------|------|-------|--------|
| R1.1 | `proc_sched.s` pick_next/timer | VM step | `r11-sched/timer` | VERIFIED via suite |
| R1.2 | `proc_telem.s` counters | — | `r12-telem` | VERIFIED |
| R1.3 | — | `telem_inspect.c` | `r13-telem-inspect` | VERIFIED |
| R2.0 | `proc_policy.s` policy_select | — | `r20-policy` | VERIFIED |
| R2.1 | — | `policy_verify.c` | `r21-*` | VERIFIED |
| R2.2 | — | `eval_*.c` | `r22-*` | VERIFIED |
| R3.0 | FIFO via ready_seq | fcfs images | `r30-*` | VERIFIED |
| R3.1 | — | `pval_*.c` | `r31-*` | VERIFIED |
| R3.2 | — | `reconfig_*.c` | `r32-*` | VERIFIED |
| R3.3 | — | `planner_*.c` | `r33-*` | VERIFIED |
| R3.4 | ready_seq assign | sched trace + active guest | `r34-*` | VERIFIED |
| R3.5 | — | `feedback_*.c` | `r35-*` | VERIFIED |
| R3.6 | — | `opt_*.c` | `r36-*` | VERIFIED |
| R3.7 | — | `robust_*.c` | `r37-*` | VERIFIED |

## B. Architecture verification

**Guest kernel (HAEUN-32 in HXF):** process-v1 PCB table `0xA000`, policy `0xA474`, ready_seq globals `0xA46C/A470`, wrap `0xA478`. Scheduler: trap/timer → `pick_next` → `policy_select` → switch. Telemetry updates on READY/RUNNING paths.

**Host VM:** `haeun_vm_step` interprets guest; single-threaded; safe point = between steps.

**Host research tooling:** telem inspect, eval, pval, reconfig, planner, feedback, optimize, robust. These are **not** kernel-resident.

**Guest workloads:** fixed HXF catalog (`r12_*`, `r22_burst`) under `build/` and `build/fcfs/`.

## C. Evidence classes used in R4.0

- **VERIFIED:** exercised by `make test` binaries linked above.
- **OBSERVED:** measured in R3.6/R3.7 smoke campaigns (reps=2–3).
- **REPORTED:** earlier milestone docs (cross-check before citing as VERIFIED).
- **INSUFFICIENT:** inferential statistics; universal optimality.
- **NOT TESTED:** research/100-rep campaigns; SMP; parametric workload sweeps.

## D. Research risks

1. Single-threaded host interpreter; no SMP.
2. `ready_seq` wrap sticky → UNKNOWN (fail closed).
3. Small-N descriptive CI only.
4. Fixed HXF catalog; no RNG; limited variation.
5. Host tooling vs kernel boundary easily misstated.
6. R3.2 rollback = policy ID only.
7. Adaptive mid-run phase confounders if used as evidence of offline superiority.
8. Generalization beyond catalog workloads unproven.

## Terminology revision

Prefer **host-assisted autonomous policy optimization framework** over “fully autonomous OS.”  
SHADOW remains default for planner/feedback; APPLY/OPTIMIZE/ADAPTIVE are explicit opt-ins.
