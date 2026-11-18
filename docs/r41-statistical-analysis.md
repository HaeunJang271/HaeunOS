# R4.1 Statistical Analysis

## Executed samples

| Preset | n per policy×workload | Workloads | Policies | Failed runs (reported) |
|--------|----------------------|-----------|----------|------------------------|
| Smoke | 3 | 6 | 2 | 0 |
| Standard | 10 | 6 | 2 | 0 |

## Classifiers (OBSERVED)

Across all six workloads at both presets:

- Replay: EXACT_MATCH
- Reliability: SUFFICIENT (under preset min_success)
- Sensitivity: STABLE
- Verify: CONFIRMED
- Overall: PASS

## Evidence kind

| Kind | Status |
|------|--------|
| Descriptive | **OBSERVED** (n=3 and n=10) |
| Exploratory | **OBSERVED** (rankings/Pareto inside opt path) |
| Inferential significance | **INSUFFICIENT / NOT CLAIMED** |
| Confirmatory | **NOT CLAIMED** |

## Interpretation rules

- n=10 strengthens smoke but does not prove statistical superiority of RR vs FIFO.
- Deterministic HXF ⇒ EXACT_MATCH partly reflects fixed images, not stochastic robustness.
- Descriptive CI machinery exists in opt/eval code; aggregate report emphasizes classifiers here.
- Guest ticks ≠ wall-clock; do not mix units.
