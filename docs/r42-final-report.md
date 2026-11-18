# R4.2 Final Report

## Final Status: **PARTIAL PASS**

Completed: baseline audit, separate-directory clean reproduction, 6×5 profile matrix, 120/120 performance campaign, descriptive stats, evidence registry, literature re-audit, paper/presentation, kernel hash match, regression path.

Not completed / not claimed: third-party independent lab; git worktree from HEAD; per-cell live APPLY + post-apply selection-rule proof in the 6×5 matrix.

## 1. Baseline Audit — VERIFIED
`docs/r42-baseline-audit.md`. R4.1 numbers treated as REPORTED only.

## 2. Clean-room Reproduction — OBSERVED (limited)
Separate-directory working-tree copy; same host. Third-party NOT TESTED.

## 3. Multi-profile Verification — OBSERVED
30/30 cells SELECT RR; replay_same; apply/cell NOT TESTED.

## 4. Performance Comparison — OBSERVED
120 planned, 120 success, 0 fail. RR lower ready_wait means on all six workloads.

## 5. Statistical Analysis — descriptive OBSERVED; inferential NOT PERFORMED

## 6. Evidence Traceability — C1–C5 documented; P1 OBSERVED; P2 INSUFFICIENT

## 7. Related Work — metadata VERIFIED; body access as R4.1

## 8. Paper / Presentation — updated for R4.2 data

## 9. Kernel Integrity — VERIFIED match to R4.1 hashes

## 10. Regression — VERIFIED
| Command | Exit |
|---------|------|
| `make clean` | 0 |
| `make all` | 0 |
| `make test` (incl. r41 both + r42) | 0 |
| Separate-dir `r42` / `r41 smoke` | 0 / 0 |

## 11. Remaining Limitations
Uncommitted git history; deterministic catalog; two policies; no SMP; no 3rd-party lab; no per-cell apply in R4.2 matrix.

## Principle
Evidence before claims. Reproducibility before generalization. Safety before optimization.
