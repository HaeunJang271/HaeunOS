# R4.2 Performance Analysis

## Execution

| Item | Value |
|------|-------|
| Planned | 6×2×10 = **120** |
| Success | **120** |
| Fail | **0** |
| Missing | **0** |
| Host clock (tooling) | 1.290 s (primary run) |
| Design | Independent fresh VM per eval run (not paired wall-clock) |

Source: `research/experiments/r42-performance-results.json`, `r42-statistics.json`.

## Descriptive ready_wait means (guest ticks; lower better)

| Workload | RR mean | FIFO mean | Jain RR | Jain FIFO |
|----------|---------|-----------|---------|-----------|
| cpu_bound | 749 | 812 | 1 | 1 |
| short_job | 4920 | 5280 | 1 | 1 |
| mixed | 15439 | 18778 | 0.913645 | 0.913645 |
| bursty | 4600 | 4960 | 1 | 1 |
| block_sleep | 159 | 168 | 1 | 1 |
| block_wait | 216 | 254 | 0.506954 | 0.506954 |

Within each policy×workload, n=10 samples are deterministic (stddev 0 or ~1e-7). Technical CI collapses to the point estimate.

## Statistical claims

- Descriptive comparison: **OBSERVED** — RR shows lower mean ready_wait on all six catalog workloads in this build.
- Inferential significance tests: **NOT PERFORMED** (assumptions not validated; n=10 deterministic).
- General superiority / production claims: **FORBIDDEN / NOT CLAIMED**.
- Jain fairness identical RR vs FIFO per workload here → no fairness differentiation **OBSERVED** under these images.

## Confounders

Fixed HXF; image trees may differ between RR/FIFO paths; guest ticks ≠ wall-clock; deterministic catalog.
