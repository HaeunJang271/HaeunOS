# R4.3 Baseline Audit (R4.2 read-only)

Date: 2026-11-18. R4.2 results below are **prior REPORTED/OBSERVED only** — not R4.3 runs.

| R4.2 item | Prior class | R4.3 treatment |
|-----------|-------------|----------------|
| Separate-dir clean build | OBSERVED | Do not reuse as R4.3 |
| 6×5 all SELECT RR | OBSERVED | Re-map apply status in R4.3 |
| 120/120 performance | OBSERVED | Not re-run as new claim |
| RR ready_wait < FIFO | OBSERVED descriptive | Not generalized |
| Kernel hash = R4.1 | VERIFIED | Re-check in R4.3 |
| HEAD worktree | NOT TESTED | Blocked by dirty tree |
| Per-cell live APPLY | NOT TESTED | Target of R4.3 |

## Gaps R4.3 addresses

1. Immutable Git baseline / HEAD worktree (or explicit block reason).
2. Behavioral selection proof (expected slot vs observed), not policy ID alone.
3. Bidirectional active apply with selection-rule checks.
4. Safety rejection + rollback evidence refresh.
5. External reproduction package (3rd party still NOT TESTED until external run).
