# R4.1 Experiment Design

## Objectives

Strengthen R4.0 evidence for reproducibility, recommendation verification, and sensitivity under larger-N **standard** preset without inventing new policies/workloads.

## Factors

| Factor | Levels |
|--------|--------|
| Workload | cpu_bound, short_job, mixed, bursty, block_sleep, block_wait |
| Policy | RR, Preemptive FIFO |
| Profile (opt/robust) | default balanced for campaign; five profiles exist in opt |
| Reps | smoke=3; standard=10 |

## Run counts

| Campaign | Baseline matrix | Notes |
|----------|-----------------|-------|
| Smoke | 6×2×3 = **36** | Plus sensitivity/verify host-side |
| Standard | 6×2×10 = **120** | Plus sensitivity/verify |

120 runs ≠ statistical significance proof.

## Integrity

- Fresh VM per eval runner invocation (existing engine).
- Deterministic HXF catalog; **no RNG** — seeds are FNV identifiers, not stochastic variation.
- Failures not silently dropped.
- Export under `build/r41-smoke` and `build/r41-standard` (single-level dirs; `_mkdir` creates one level).
- `build_id` / `source_revision` / `config_hash` recorded.

## Metrics (semantics)

Guest instruction domain: run_ticks, ready_wait_ticks, sleep_ticks, blocked_ticks, dispatch_count, context_switches, timer_irqs, completion timestamps, Jain fairness.  
Host: `clock()` wall for tooling phases only — **not** guest latency.

## Statistical plan

Descriptive mean/median/stddev/min/max; descriptive CI when n≥2. No normality assumption for significance tests. Classify evidence as descriptive/exploratory only unless methods expand.

## Resource decision rule

1. Run smoke.
2. If smoke host wall ≪ minutes and disk free, run standard.
3. Extended/research require explicit authorization (not auto).
