# R3.6 Test Report

Status: **Demonstrated** (executed 2026-11-17).

## Suites

| Suite | Result |
|-------|--------|
| `r36-opt-unit-tests` | PASS |
| `r36-opt-guest-tests` | PASS |
| R1.1–R3.5 via `make test` | PASS |

## Unit

profiles/normalize, stats+CI, Pareto/select, state schema reject — PASS

## Guest (reps=2)

- 6 workloads OPTIMIZE (RR vs FIFO) — PASS; observed SELECT→RR (policy=0) under balanced for this catalog
- 5 objective profiles on `short_job` — PASS
- OPTIMIZE does not mutate live `0xA474` — PASS

## Final verification

| Command | Result |
|---------|--------|
| `make clean` | PASS |
| `make all` | PASS |
| `make test` | PASS |

## Not claimed

Statistical significance; Adaptive superiority; universal profile optimality; ML.
