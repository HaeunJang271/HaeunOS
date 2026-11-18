# R3.6 Objective Model

Experimental profiles (not universal quality definitions).

| Profile | Primary weights |
|---------|-----------------|
| balanced | ready 0.40, jain 0.30, dispatch 0.20, completed 0.10 |
| fairness | jain 0.70, ready 0.30 |
| latency | ready 0.75, dispatch 0.25 |
| throughput | completed 0.60, ready 0.40 |
| low_overhead | dispatch 0.70, ready 0.30 |

Normalization: pairwise min-max so higher normalized value is better under direction.
Missing metrics stay unavailable; never zero-filled.
`min_improve` default 0.05 relative score gap.
