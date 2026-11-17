# R2.2 Workload Specification

Deterministic guest workloads for host-side scheduler evaluation.
All use existing process-v1 ECALLs. **No kernel changes.**

## Catalog

| Name | Category | Image | Exit | Min PIDs | Seed/RNG |
|---|---|---|---|---|---|
| `cpu_bound` | cpu | `r12_run` | 21 | 1 | seed=0, no RNG |
| `short_job` | short | `r12_yield` | 22 | 1 | seed=0, no RNG |
| `mixed` | mixed | `r12_pair` | 25 (final r1; parent EXIT 24 earlier) | 2 | seed=0, no RNG |
| `bursty` | bursty | `r22_burst` | 26 | 1 | seed=0, no RNG |
| `block_sleep` | blocking | `r12_sleep` | 23 | 1 | seed=0, no RNG |
| `block_wait` | blocking | `r12_wait` | 5 | 2 | seed=0, no RNG |

Default step caps are defined in `src/eval_workload.c`.

## Category intent

- **CPU-bound:** long spin, minimal yield — sustained RUN accounting.
- **Short-job:** many voluntary YIELDs then EXIT — self-dispatch / short tasks.
- **Mixed:** parent+child alternating YIELD — heterogeneous multiprogramming.
- **Bursty:** spin then YIELD cycles (`r22_burst`) — intermittent demand.
- **Blocking:** SLEEP or WAIT — sleep/blocked ticks (not device I/O).

## Unsupported

- Real device / network I/O blocking workloads.
- More than three address-space slots.
- Non-deterministic seeds (catalog workloads have `uses_rng=0`).

## Build

```text
python tools/build_r22.py build
```
