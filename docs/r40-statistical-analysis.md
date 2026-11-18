# R4.0 Statistical Analysis

## Methods (implemented)

- Sample mean/median/min/max/stddev (R2.2)
- Descriptive CI ≈ mean ± 1.96·s/√n when n≥2 (R3.6/R3.7)
- Pairwise normalized weighted scores (R3.6)
- Pareto on ready/jain/dispatch (R3.6)
- Sensitivity weight perturbation (R3.7)
- Reliability / verify classifiers (R3.7)

## Distinctions

| Kind | Status in R4.0 |
|------|----------------|
| Descriptive | OBSERVED under smoke (reps=3, 2026-11-18) |
| Exploratory | OBSERVED rankings / Pareto inside opt path |
| Inferential significance | **INSUFFICIENT / NOT CLAIMED** |

### Smoke campaign (executed)

- n=3 per policy×workload under SMOKE preset; config_hash `0xab1dcbe0`.
- Replay: EXACT_MATCH on all six workloads → reproducibility classifier PASS at smoke scale.
- Sensitivity: STABLE for all six under implemented weight perturbations.
- Verify: CONFIRMED for all six (recommendation check non-destructive).
- Reliability classifier: SUFFICIENT under preset min_n for this smoke.
- Standard/extended/research (n=10/30/100): **NOT TESTED**.

Guest ticks ≠ wall-clock. Overlapping CIs ≠ automatic significance test.
