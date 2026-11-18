# R3.6 Statistics

Reuses R2.2 `haeun_eval_stats_compute` (mean/median/min/max/sample stddev).

Optional descriptive CI: `mean ± 1.96·s/√n` when n≥2 and stddev available.
**Assumes approximate normality only as a descriptive interval — not a significance test.**

Insufficient samples → CI unavailable → may yield INCONCLUSIVE / EQUIVALENT.
Invalid/missing runs counted separately; never discarded silently from reports.
