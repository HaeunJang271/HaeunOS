# R3.7 Preliminary Source Audit

Status: **Audit complete** (no kernel edits). Date: 2026-11-17.

## 1. Reusable capabilities

| Layer | Reuse |
|-------|-------|
| R2.2 | `haeun_eval_run_once` / `experiment_run`; per-run fresh RAM+VM; metric optionals |
| R3.1–R3.2 | `haeun_pval_validate`, `haeun_recfg_*`, FI points |
| R3.4 | active guest tests / sched trace observe |
| R3.5 | feedback validity semantics |
| R3.6 | profiles, pairwise score, Pareto, `haeun_opt_stats_fill` (descriptive CI) |

## 2. Statistical limitations (existing)

- R2.2/R3.6 CI: approximate normal `mean±1.96·s/√n` when n≥2 — **descriptive**, not a significance test.
- No paired difference CI; no effect-size library.
- R3.6 guest tests used reps=2; default config reps=3.
- Overlapping CIs must not be treated as automatic significance.

## 3. Sources of nondeterminism

- Guest images are **intrinsically deterministic** (fixed HXF); no RNG in catalog.
- Host wall-clock / stderr buffering does not affect guest metrics.
- `g_busy` / FI statics in R3.2 if not cleared between txns (tests clear).
- Adaptive mid-run sampling (R3.5) has phase confounders — R3.7 OPTIMIZE path uses offline `eval_runner` only.

## 4. Experiment contamination risks

- Shared static FI / recfg busy flag if APPLY/stress interleaved without clear.
- Reusing one VM across candidates without reload (runner already isolates).
- Treating fail_count as success or missing→0 (must not).

## 5. Missing reproducibility guarantees (pre-R3.7)

- No experiment configuration hash / seed field in R3.6 JSON.
- No EXACT_MATCH vs METRIC_MATCH classification.
- No duplicate-ID / incomplete-set detector.
- No sensitivity / robustness classifiers.
- No systematic reconfiguration stress suite beyond R3.4 demos.

## 6. Runtime safety risks

- Direct `0xA474` writes outside R3.2.
- Treating UNKNOWN as PASS.
- Claiming full-VM rollback.
- APPLY implied by optimization.

## 7. Must remain unchanged

Full SHA-256 (R3.6 baseline = current):

| File | SHA-256 |
|------|---------|
| `proc_sched.s` | `f378293d24ef74fe9af42615959e40c5c1de038d49a38590adfa562bf200d7d7` |
| `proc_policy.s` | `38a79c49d2f9f9d72589f2c13e63f96c1b279bb051e02ac8bb1328f7d81299de` |
| `proc_trap.s` | `e6cb2a9823a5e6a4d3bcba1322fcaad4e4732fb3138972df7cef672ebe3bc020` |
| `proc_telem.s` | `2d7dbeacdb4326838487a580f946de660a39c141c408e6767d529a89c06a705a` |
| `proc_fork.s` | `d895e560f52a33610fb9f63271d854bb38d7941422bc9a4eb093f57ed60ca603` |

No PCB/ECALL/policy ABI changes. Host-only R3.7.

## Workload seed semantics

Catalog workloads have **no random generator**. Effective seed =
`FNV-1a(workload_id || policy_id || step_cap || image_name)`.
Same seed ⇒ same image path + step_cap. Parametric variation beyond step_cap
is **NOT_SUPPORTED** unless a future generator exists.

## Proposed R3.7 modules (host)

`robust_*`: presets, runner, replay, reliability, sensitivity, robustness
(variation report), reconfig stress, R3.6 verify, report/CLI/persistence.
