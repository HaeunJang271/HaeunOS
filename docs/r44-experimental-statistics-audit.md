# R4.4 Experimental & Statistics Audit

**Baseline:** `386c3a1517c49be8320c6ea263e726a6c006bb53`  
**Do not regenerate or rewrite prior experiment JSON in this audit.**

## Evidence layers (definitions)

| Layer | Meaning |
|-------|---------|
| Planned | Protocol stated in docs |
| Executed | Suites ran in a recorded campaign |
| Log-confirmed | Console/log lines available |
| Doc-only | Narrative without fresh artifact |
| Reproduced | Same-host worktree rebuild/test (R4.3.1/2) |
| Independent | Third-party — **NOT TESTED** |

## Campaign inventory

| Campaign | Planned | Executed | Log-confirmed | Reproduced | Independent |
|----------|---------|----------|---------------|------------|-------------|
| R4.0 smoke 6×2×n=3 | YES | YES (prior) | PARTIAL (reports) | Via `make test` suites | NOT TESTED |
| R4.1 standard 6×2×10 | YES | YES | Prior reports + worktree r41 pass | Same-host worktree | NOT TESTED |
| R4.2 performance 120 | 6×2×10=120 | 120 ok / 0 fail | `r42-performance-results.json` | Worktree re-ran r42 tests | NOT TESTED |
| R4.2 profiles 30 cells | 6×5 | 30 SELECT RR | `r42-profile-verification.json` | Worktree log | NOT TESTED |
| R4.3 active apply | RR↔FIFO | YES | `r43-active-apply.json`; 61/61 lines | Worktree log | NOT TESTED |
| R4.3 safety/rollback | YES | YES | JSON artifacts | Covered in r43 tests | NOT TESTED |
| Per-cell live apply ×30 | Planned as mapping | **NOT** | `apply_path=NOT_TESTED_per_cell` | — | — |

Worktree reproduction (`docs/r432-reproduction-evidence.md`): `CHAIN_EXIT_0; INDIVIDUAL_EXIT_CODES_NOT_CAPTURED`. Does **not** re-author original R4.2 JSON as a new primary result.

## Descriptive performance (R4.2)

Source: `docs/r42-performance-analysis.md`, `research/experiments/r42-statistics.json`.

- Metric: guest `ready_wait` ticks (lower better).
- RR mean < FIFO mean on all six catalog workloads (**OBSERVED**).
- Within policy×workload, n=10 often **deterministic** (stddev ~0) → technical CI collapses.
- Jain fairness identical RR vs FIFO per workload here → **no fairness differentiation OBSERVED**.

## Statistical methods audit

| Topic | Finding | Grade |
|-------|---------|-------|
| Mean / SD computation | Present in opt/eval tooling | VERIFIED (code exists) |
| Sample independence | Fresh VM per run claimed; HXF deterministic | PARTIAL |
| Deterministic repeats | Exact replay expected; not stochastic robustness | OBSERVED limitation |
| Effect size | Computable descriptively; not formalized as primary claim | INSUFFICIENT for paper “effect” |
| Inferential tests | **NOT PERFORMED** | INSUFFICIENT |
| n=10 ⇒ significance | **Forbidden leap** | NOT CLAIMED |
| RR general superiority | Catalog-only descriptive | Cannot claim generally |
| FIFO “inferior” | Same limitation | Cannot claim generally |

## Policy note

Only **RR=0** and **preemptive FIFO=1**. Do not call FIFO non-preemptive FCFS. Profile campaign recommended RR in **30/30** cells — optimizer diversity across profiles is **weak** under this catalog.

## Publication rules for stats

1. Prefer “descriptive comparison on fixed catalog HXF”.
2. Disclose determinism and guest-tick units.
3. Do not cite p-values that were never computed.
4. Separate safety/apply evidence from performance tables.
