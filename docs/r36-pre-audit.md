# R3.6 Preliminary Source Audit

Status: **Audit complete** (no kernel edits). Date: 2026-11-17.

## 1. Existing candidate policies

| ID | Name | Semantics |
|----|------|-----------|
| 0 | RR | Round-robin among READY/RUNNING |
| 1 | Preemptive FIFO | Oldest READY via `ready_seq` (policy word still named FCFS) |

No additional kernel policies without new contracts. R3.6 scope: **RR + FIFO only**.

## 2. Optimization parameters available

R3.5 `haeun_fb_config` weights/thresholds; R3.3 dwell/cooldown; R2.2 cost weights.
No quantum/priority ABI today → adaptive-quantum / MLFQ / SRTF = **future work**.

## 3. Evaluation metrics

R2.2: run/ready/sleep/blocked/dispatch/ctx/timer, Jain on run_ticks, optional completion.
Validity via `haeun_eval_opt.available` / R3.5 `haeun_fb_validity`. Missing ≠ 0.

## 4. Feedback interfaces

`haeun_fb_sample_*`, `haeun_fb_evaluate`, `haeun_fb_cycle`, `haeun_fb_exp_fixed/adaptive`,
state save/load. Outcomes: IMPROVED/DEGRADED/NEUTRAL/INCONCLUSIVE.

## 5. Workload generators

Catalog: `cpu_bound`, `short_job`, `mixed`, `bursty`, `block_sleep`, `block_wait`
(`eval_workload.c`). Labels are catalog IDs, not runtime proof.

## 6. Reproducibility

Deterministic guest images; fixed step caps; host single-threaded.
R3.5 default experiments used **1 rep** (documented non-significance).

## 7. Experimental limitations

No CI in R2.2 stats; Adaptive mid-run phase confounders; guest ticks ≠ wall-clock;
Pareto not implemented; optimize≠apply not separated as named modes.

## 8. Proposed optimization architecture (host-only)

```
Characterize → Candidates{RR,FIFO} → Experiment(R2.2/R3.5)
→ Stats(+CI when n≥2) → Pareto → Optimizer(profile)
→ Recommendation → (APPLY only) R3.1→R3.2→R3.4; Feedback R3.5
```

Modes: SHADOW (default report), OPTIMIZE (explore), APPLY (explicit).

## 9. Expected source changes

`include/haeun/opt_*.h`, `src/opt_*.c`, CLI in `main.c`, Makefile R36 tests, `docs/r36-*.md`.

## 10. Must remain unchanged

| File | SHA-256 prefix |
|------|----------------|
| `proc_sched.s` | `F378293D24EF74FE` |
| `proc_policy.s` | `38A79C49D2F9F9D7` |
| `proc_trap.s` | `E6CB2A9823A5E6A4` |
| `proc_fork.s` | `D895E560F52A3361` |
| `proc_telem.s` | `2D7DBEACDB432683` |

No new ECALLs; no direct `0xA474` from optimizer; UNKNOWN≠PASS.
