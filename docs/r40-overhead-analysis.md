# R4.0 Overhead Analysis

## Measurement plan

| Cost | Method |
|------|--------|
| Guest execution | guest steps / run_ticks (instruction domain) |
| Host research tooling | `clock()` around `haeun_rb_run_experiments` in `r40_integration_tests` |
| Safety/txn | included in reconfig paths; not separately timed in R4.0 smoke |

## Results (OBSERVED, 2026-11-18)

| Quantity | Value | Method | Notes |
|----------|-------|--------|-------|
| Host tooling wall (smoke experiments) | **0.271 s** | `clock()` / `CLOCKS_PER_SEC` | Single sample around `haeun_rb_run_experiments` only |
| Sample count | 1 | — | No mean/median/stddev across hosts |
| Guest tick cost | per-run in eval traces | guest instruction domain | Not wall-clock |

```
host_tooling_wall_s=0.271 (clock(); research-tool overhead)
```

## Separation

| Layer | Status |
|-------|--------|
| Guest execution cost | Measured as guest ticks/steps in eval (not re-tabulated here) |
| Host VM overhead | Implicit in step loop; not isolated in R4.0 |
| Host research-tool overhead | OBSERVED 0.271 s for smoke experiment phase |

## Interpretation rules

- Host tooling overhead is **not** kernel scheduling overhead.
- Guest ticks are not wall-clock latency.
- One `clock()` sample is exploratory, not a benchmark claim.
- Pareto/sensitivity/export times were not separately instrumented → **NOT TESTED** as isolated costs.
