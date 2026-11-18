# R3.5 Test Report

Status: **Demonstrated** (executed 2026-11-17).

## Suites

| Suite | Result |
|-------|--------|
| `r35-feedback-unit-tests` | PASS |
| `r35-feedback-guest-tests` | PASS |
| R1.1–R3.4 regression via `make test` | PASS |

## Unit (executed)

- metrics_and_fairness — PASS
- compare_and_outcomes (IMPROVED + fingerprint mismatch INCONCLUSIVE) — PASS
- gates_and_state (cooldown, save/load, bad schema) — PASS
- shadow_no_commit_flag — PASS

## Guest (executed)

- shadow_unchanged: `txn_committed==0`, policy word unchanged — PASS
- all 6 workloads × Fixed RR + Fixed FIFO + Adaptive(SHADOW) — PASS (`transitions==0`)

Workloads: `cpu_bound`, `short_job`, `mixed`, `bursty`, `block_sleep`, `block_wait` (1 rep each; **not** statistical proof).

## Final verification

| Command | Result |
|---------|--------|
| `make clean` | PASS |
| `make all` | PASS |
| `make test` | PASS |

## Not claimed

Adaptive superiority; statistical significance; ML.
