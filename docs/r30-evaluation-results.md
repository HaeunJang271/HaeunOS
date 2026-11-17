# R3.0 Evaluation Results

Variant evaluated: **preemptive FIFO by `ready_since`** (`fcfs_preemptive_fifo`), not classical non-preemptive FCFS.

Commands (executed):

```text
python tools/build_r30.py build
./build/haeun-eval.exe build <workload> 1 --policy rr
./build/haeun-eval.exe build <workload> 1 --policy fcfs
```

## Summary table (1 rep each; guest ticks)

| Workload | Policy | total_run | ready_wait | Jain(run) |
|---|---|---:|---:|---:|
| cpu_bound | RR | 1954 | 665 | 1.0 |
| cpu_bound | FCFS | 1954 | 700 | 1.0 |
| short_job | RR | 6891 | 4440 | 1.0 |
| short_job | FCFS | 6891 | 4640 | 1.0 |
| mixed | RR | 41549 | 28824 | 0.914 |
| mixed | FCFS | 41549 | 33678 | 0.914 |
| bursty | RR | 8891 | 4120 | 1.0 |
| bursty | FCFS | 8891 | 4320 | 1.0 |
| block_sleep | RR | 260 | 147 | 1.0 |
| block_sleep | FCFS | 260 | 152 | 1.0 |
| block_wait | RR | 22561 | 406 | 0.507 |
| block_wait | FCFS | 22561 | 474 | 0.507 |

## Interpretation

- Single-run observational deltas only; not statistical significance.
- `total_run_ticks` matched across policies for these workloads.
- FCFS path has more selection instructions → modestly higher `ready_wait` is expected and **not** a performance claim either way.
- Lower context-switch count is not used here as a quality metric.
- Cost-function scores are experimental and not real-world predictions.

## Limitations

See `docs/r30-policy-safety.md` and `docs/r22-metrics-and-statistics.md`.
