# R4.7 W2 Step-Cap Amendment (R4.7.51)

**Status:** Prospective append-only amendment to the W2 parameter freeze.  
**Does not modify:** [`docs/r47-w2-parameter-freeze.md`](r47-w2-parameter-freeze.md) or the original R4.7 specification.  
**Does not erase:** the preserved 300,000-step failure evidence under `research/experiments/r47-discrimination/results/gate-d1/failure-300k/`.

## 1. Change

| Item | Value |
|------|-------|
| Original approved step cap | **300,000** |
| Newly approved step cap | **800,000** |
| Scope of change | **Step cap only** |

User approval for 800,000 was received **before** resumed confirmatory repetitions (R4.7.51).

## 2. Failure and diagnostic record (not confirmatory results)

| step_cap | Observed |
|----------|----------|
| 300,000 (original freeze) | `fail_reason=step cap exceeded`, `completed=0` |
| 400,000 (diagnostic) | same failure |
| 500,000 (diagnostic) | same failure |
| ≥750,000 (diagnostic) | successful completion, `completed=3`, parent EXIT 31 |

The 300,000-step failure artifacts remain preserved and are **not** part of the 800,000-step confirmatory result set.

## 3. Unchanged parameters

- Processes: L, S1, S2 (3)
- CPU workload: exactly 12,000 pure-CPU loop iterations per process
- Creation: L forks S1, then S2; no intentional yield between forks
- READY gate: L `sys_yield` after both forks
- Completion: L WAIT×2, EXIT 31; S1 EXIT 32; S2 EXIT 33
- Repetitions: 10 per policy
- Profiles: existing five only
- RR=0, FIFO=1
- Predeclared epsilon rules unchanged
- No policy winner predicted

## 4. Confirmatory campaign identity

Confirmatory outputs live under `research/experiments/r47-discrimination/results/gate-d1/` with names distinct from `failure-300k/`.
