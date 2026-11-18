# R3.3 Decision Model

## Experimental thresholds (`haeun_plan_config`)

| Parameter | Default | Role |
|---|---|---|
| `ready_run_ratio_high` | 2.0 | Bursty / queue pressure interest |
| `sleep_share_high` | 0.45 | Blocking/sleep classification |
| `dispatch_run_high` | 0.15 | Short-job interest |
| `min_dwell_ticks` | 1000 | Stability |
| `cooldown_ticks` | 500 | Stability |
| `consecutive_needed` | 2 | Duplicate suppress |
| `hysteresis_delta` | 0.25 | Noise band |
| `plan_ttl_ticks` | 100000 | Expiry |
| `experimental_rules` | 1 | Enable recommend rules |

These are **configurable experimental defaults**, not proven optima.

## Rules (summary)

1. Missing/invalid monitor → NO_DECISION
2. UNKNOWN workload → NO_DECISION
3. IDLE → KEEP_CURRENT
4. Short-job under RR → interest in Preemptive FIFO (experimental)
5. High ready/run under RR → FIFO interest (experimental)
6. CPU/mixed under FIFO → RR interest (experimental)
7. Else KEEP_CURRENT
8. Stability may demote interest to KEEP_CURRENT
9. Safety may demote RECOMMEND_CHANGE → REJECTED_BY_SAFETY

## Metric limits

- Guest ticks ≠ wall-clock
- Dispatch ≠ useful throughput
- Jain fairness (R2.2) not used as sole selector here
- Small samples ≠ significance
