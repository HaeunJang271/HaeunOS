# R4.2 Baseline Audit (R4.1 read-only)

Date: 2026-11-18. Method: re-read R4.1 docs + source; recompute kernel SHA-256.  
**R4.1 experiment numbers below are prior-run records only — not R4.2 results.**

## 1. R4.1 report vs artifacts

| Item | R4.1 report | Working-tree artifacts | Status |
|------|-------------|------------------------|--------|
| smoke hash | `0xab1dcbe0` | Prior `build/r41-smoke` may be absent after clean | **REPORTED** (prior) |
| standard hash | `0x4a32cc07` | Same | **REPORTED** (prior) |
| 6× EXACT/STABLE/CONFIRMED | documented | prior campaign log | **REPORTED** |
| eval fail=0 | documented | prior | **REPORTED** |

R4.2 must not treat these as newly executed.

## 2. Smoke vs standard

- Smoke: preset=0, reps=3, baseline 36 runs.
- Standard: preset=1, reps=10, baseline 120 runs.
- Distinct `config_hash` (includes reps/preset/profile).

## 3. Config hash rule

`haeun_rb_config_hash` over build_dir, reps, profile, preset (`robust_types` / `robust_types.c`). Changing reps changes hash (observed R4.1: smoke≠standard).

## 4. Experiment data provenance

R4.1 outputs from `r41_validation_tests` / robust export under `build/r41-*`. Deterministic HXF catalog; seeds are FNV identifiers.

## 5. Policy / workload paths

`haeun_opt_run_workload` → `haeun_eval_experiment_run` for policy 0 (RR) and 1 (FIFO) per catalog kind. Images under `build/` and `build/fcfs/`.

## 6. Optimizer vs adaptive feedback

- Optimizer (`opt_*`): offline/host candidate evaluation + recommendation; SHADOW default.
- Feedback (`feedback_*`): runtime monitor/adapt loop; must not bypass pval/txn for APPLY.

## 7. Safety / transaction call path

Recommendation → (optional) `haeun_opt_apply` → planner safety → `pval` → `reconfig_txn`. Optimizer does not write `0xA474` directly. UNKNOWN ≠ PASS.

## 8. Kernel hashes (working tree vs R4.1 baseline)

| File | R4.1 SHA-256 | Current | Match |
|------|--------------|---------|-------|
| proc_sched.s | f378293d…d7d7 | f378293d…d7d7 | YES |
| proc_policy.s | 38a79c49…99de | 38a79c49…99de | YES |
| proc_trap.s | e6cb2a98…c020 | e6cb2a98…c020 | YES |
| proc_telem.s | 2d7dbeac…705a | 2d7dbeac…705a | YES |
| proc_fork.s | d895e560…a603 | d895e560…a603 | YES |

Note: `git status` may show kernel files modified vs last **commit** (uncommitted R3.x+ research work). R4.2 integrity gate is the **R4.1 research baseline hash**, which matches. No R4.2 kernel edits.

## What R4.2 must not change

Kernel process sources; policy set; PCB/ECALL ABI; UNKNOWN semantics; historical R4.1 JSON.
