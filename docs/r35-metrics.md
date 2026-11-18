# R3.5 Metrics

Built on R2.2 `haeun_eval_run_metrics`. Missing never becomes zero.

## Sample fields

| Metric | Source | Notes |
|--------|--------|-------|
| total_run | sum run_ticks | guest ticks ≠ wall-clock |
| avg_ready_wait | optional avg | UNAVAILABLE if missing |
| max_ready_wait | max slot ready_wait | INSUFFICIENT if no participants |
| jain_fairness | Jain on run_ticks | allocation fairness only |
| dispatch_count | total_dispatch | coarse overhead proxy |
| ctx_switches | host/guest count | when available |
| timer_irqs | timer IRQ count | when available |
| completed | completed process count | |
| completion_latency | complete_lo based | UNAVAILABLE if no completions |

## Validity

`VALID` | `UNAVAILABLE` | `INVALID` | `INSUFFICIENT_SAMPLE`
