# R3.5 Evaluation Methodology

## Modes compared

1. Fixed RR (`policy_id=0` images)
2. Fixed FIFO (`build/fcfs/*`, `policy_id=1`)
3. Adaptive (SHADOW by default in harness)

Same R2.2 workload catalog and image paths. Adaptive does not claim superiority.

## Statistics

Per experiment: reps, success/fail, mean of available avg_ready_wait / Jain / dispatch when samples exist.

**Repetitions are not statistical significance.** No confidence intervals claimed unless explicitly computed and documented. Default smoke uses 1 rep.

## Comparability

Before/after feedback compare requires matching `workload_id`, fingerprint, and participant count. Mismatch → INCONCLUSIVE.

## Confounders

- Guest ticks ≠ wall-clock.
- Adaptive mid-run samples share one VM lifetime (phase effects).
- Catalog workload label ≠ proven runtime class.
- Trace is host-visible approximation.
