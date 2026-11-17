# R2.2 Scheduler Policy Evaluation Framework

Host-side measurement foundation for process-v1 RR. **No scheduler optimization.
No new policies. No runtime switching.**

## Architecture

```text
workload catalog → eval_runner (load HXF, step VM)
                 → telem_inspect (read-only)
                 → metrics / fairness / stats / cost
                 → text + JSON reports / compare
```

Guest kernel scheduling paths are untouched. The evaluation cost function is
analysis-only and is not wired to `policy_select`.

## Why R2.2 does not modify the scheduler

Correct measurement requires a stable baseline. Changing `pick_next`, timer
quantum, PCB layout, or policy selection would invalidate R1.1–R2.1 results.

## How to run (tested)

```text
python tools/build_r22.py build
make build/r22-eval-unit-tests.exe
make build/r22-eval-runner-tests.exe
make build/haeun-eval.exe
./build/r22-eval-unit-tests.exe
./build/r22-eval-runner-tests.exe build
./build/haeun-eval.exe build short_job 3
./build/haeun-eval.exe build mixed 1 --json
```

Full regression:

```text
make clean
make all
make test
```

## Reproducibility

Catalog workloads set `uses_rng=0` and `seed=0`. Identical images + step caps
yield identical telemetry aggregates across repetitions (verified in runner tests).

## Export / compare

JSON schema_version=1 includes experiment metadata, per-run results, aggregates,
and unavailable-metric reasons. `haeun_eval_compare` checks workload identity
and reports observational deltas under RR only.

## Related docs

- `docs/r22-evaluation-audit.md`
- `docs/r22-workload-specification.md`
- `docs/r22-metrics-and-statistics.md`
