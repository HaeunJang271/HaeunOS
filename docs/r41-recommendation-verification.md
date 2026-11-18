# R4.1 Recommendation Verification (vs R3.6)

Historical R3.6 reports are **not rewritten**. This is a separate R4.1 verification record from `haeun_rb_verify_recommendations` under smoke/standard exports.

| Workload | Profile (campaign) | Verify status | Sensitivity | Reliability | Notes |
|----------|-------------------|---------------|-------------|-------------|-------|
| cpu_bound | balanced (0) | CONFIRMED | STABLE | SUFFICIENT | Smoke+standard |
| short_job | balanced (0) | CONFIRMED | STABLE | SUFFICIENT | |
| mixed | balanced (0) | CONFIRMED | STABLE | SUFFICIENT | |
| bursty | balanced (0) | CONFIRMED | STABLE | SUFFICIENT | |
| block_sleep | balanced (0) | CONFIRMED | STABLE | SUFFICIENT | |
| block_wait | balanced (0) | CONFIRMED | STABLE | SUFFICIENT | |

`original_policy` / `verified_policy` fields in JSON are 0 in aggregate export for these runs (balanced profile decision encoding); treat verify enum as authoritative for R4.1 status.

## Quality caveats

- Stability under weight perturbation ≠ optimality.
- Campaign used default balanced profile; per-profile five-way verification beyond classifier path is not separately tabulated here (**INSUFFICIENT** as full multi-profile ledger).
- CONFIRMED means evidence classifiers passed, not that FIFO/RR is universally best.
