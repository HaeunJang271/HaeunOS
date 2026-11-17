# R2.2 Metrics and Statistics

Host-side definitions only. Values come from `haeun_telem_read` (R1.3).

## Participation rules

Include a slot when it has a valid address space (slots 0–2) and is not an empty
stale FREE (`pid==0`, `complete_valid==0`). Slot 3 is never included.

Unavailable metrics use `available=0` with a reason string — never silent zero.

## Execution aggregates

| Metric | Definition |
|---|---|
| total_run_ticks | sum `run_ticks` over participants |
| total_ready_wait | sum `ready_wait_ticks` |
| total_sleep / total_blocked | respective sums |
| total_dispatch | sum `dispatch_count` |
| context_switches / timer_irqs | globals from PCB page |

## Derived

| Metric | Formula | Guard |
|---|---|---|
| avg_ready_wait | total_ready_wait / participants | participants > 0 |
| ctx_dispatch_ratio | context_switches / total_dispatch | total_dispatch > 0 |

## Fairness

Jain index on included `run_ticks` (zeros included):

`J = (sum x_i)^2 / (n * sum x_i^2)`

Interprets **CPU allocation** fairness for that run, not completion fairness.
A single index does not prove scheduler fairness.

## Statistics

Over successful runs only (failures counted separately, never silent-dropped):

mean, median, min, max, sample stddev (`n-1`), sample_count.

Empty sample → all opts unavailable. `n<2` → stddev unavailable.

Default repetitions: **3** (`HAEUN_EVAL_DEFAULT_REPS`) — enough to show
deterministic reproducibility in an engineering harness; not statistical power.

## Cost function (experimental)

Weighted sum of normalized components (ready wait, ctx ratio, run imbalance,
incomplete count). Default weights are demonstration-only. Output is marked
`experimental` and is **not** connected to `policy_select` / `pick_next`.

## Limitations

- Guest ticks ≠ wall clock.
- `run_ticks` includes kernel time while RUNNING.
- 32-bit modular counters.
- `dispatch_count` includes self-selection; `context_switches` only index changes.
- `ready_wait_ticks` not an exact cross-milestone invariant (policy instruction growth).
