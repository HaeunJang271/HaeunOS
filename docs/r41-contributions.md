# R4.1 Refined Contributions

## C1. Policy abstraction

**Statement:** Separation of scheduler policy selection from core dispatch for RR and preemptive FIFO.

| Aspect | Evidence |
|--------|----------|
| Implementation | `proc_policy.s`, policy word `0xA474` |
| Tests | r20, r30 |
| Experiments | Catalog RR vs FIFO |
| Prior research | Lottery (S09), Exokernel policy at app level (S12) |
| Difference | Narrow two-policy guest mechanism, not proportional-share or app-level OS |
| Strength | **VERIFIED** |
| Limits | Two policies; single-threaded |

*Reframe if needed:* engineering + research-platform contribution, not a new scheduling theory.

## C2. Safety-gated runtime reconfiguration

**Statement:** Validation-gated transaction for policy changes under between-step SP assumptions.

| Aspect | Evidence |
|--------|----------|
| Implementation | `pval_*`, `reconfig_txn.c` |
| Tests | r31–r34, r37 stress |
| Prior | K42/Soules (S03–S04), Oreizy (S07) |
| Difference | Fail-closed UNKNOWN; policy-ID-scoped rollback |
| Strength | **VERIFIED** under documented assumptions |
| Limits | No SMP; no full state migration |

## C3. Host-assisted policy optimization

**Statement:** Host framework evaluates candidates across workloads/profiles and emits recommendations without direct kernel writes.

| Aspect | Evidence |
|--------|----------|
| Implementation | `opt_*`, planner, feedback |
| Tests | r33, r35, r36 |
| Prior | Rainbow external control (S06); autonomic vision (S01) |
| Difference | OS scheduling-policy candidates on HAEUN guest; SHADOW default |
| Strength | **VERIFIED** (mechanism); performance superiority **INSUFFICIENT** |
| Limits | Host-only; fixed catalog |

## C4. Reproducible optimization evaluation

**Statement:** Preset campaigns, config hashes, replay classification.

| Aspect | Evidence |
|--------|----------|
| Implementation | `robust_*` |
| Tests/exp | R3.7/R4.0/R4.1 smoke (+ standard if run) |
| Prior | Collberg repeatability (S11) |
| Difference | Concrete replay fingerprints for this stack |
| Strength | **OBSERVED** at smoke; strengthened if standard passes |
| Limits | Deterministic images; no RNG |

## C5. Evidence-aware recommendation validation

**Statement:** Distinguishes CONFIRMED / CONDITIONAL / NOT_CONFIRMED / INSUFFICIENT / INVALID.

| Aspect | Evidence |
|--------|----------|
| Implementation | `robust_verify.c`, sensitivity |
| Tests | r37/r40/r41 verify |
| Prior | Limited in classical OS papers |
| Difference | Explicit enums tied to experiment state |
| Strength | **OBSERVED** |
| Limits | Classifier ≠ optimality proof |

## Dropped / demoted claims

- “Autonomous production OS” — demoted; use host-assisted AROS.
- “First adaptive OS” — rejected.
- Inferential policy superiority — **INSUFFICIENT**.
