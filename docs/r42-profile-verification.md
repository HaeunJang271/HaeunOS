# R4.2 Multi-Profile Verification

Fresh run via `r42_validation_tests` (n=10 experiments once, then re-score).  
JSON: `research/experiments/r42-profile-verification.json`.

## Matrix (recommended policy)

All 30 cells: **SELECT RR** (`recommended_policy=0`), `replay_same=true`.

| Workload | Balanced | Fairness | Latency | Throughput | Low overhead |
|----------|----------|----------|---------|------------|--------------|
| cpu_bound | RR | RR | RR | RR | RR |
| short_job | RR | RR | RR | RR | RR |
| mixed | RR | RR | RR | RR | RR |
| bursty | RR | RR | RR | RR | RR |
| block_sleep | RR | RR | RR | RR | RR |
| block_wait | RR | RR | RR | RR | RR |

## Criteria status

| Criterion | Status |
|-----------|--------|
| Matches optimizer `haeun_opt_select` | **OBSERVED** |
| Candidates include RR+FIFO | **VERIFIED** |
| Re-score replay identical | **OBSERVED** |
| Safety UNKNOWN≠PASS contract | **VERIFIED** (code); per-cell live pval **NOT TESTED** |
| Transaction apply per cell | **NOT TESTED** (see r34 for path **REPORTED**) |
| Post-apply selection-rule match | **NOT TESTED** here; r20/r30/r34 **REPORTED** |

Interpretation: under this catalog and profiles, weighted scores prefer RR (lower ready_wait). This is **not** proof RR is universally optimal.
