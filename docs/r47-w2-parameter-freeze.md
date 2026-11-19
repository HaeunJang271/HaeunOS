# R4.7 W2 Parameter Freeze Addendum

**Status:** User-approved experimental design freeze (R4.7.50).  
**Authority:** Explicit approval before Gate D1 implementation and execution.  
**Relation to base spec:** Supplements [`docs/r47-recommendation-discrimination-spec.md`](r47-recommendation-discrimination-spec.md). Does **not** retrospectively rewrite that document.

## 1. Nature of this freeze

These W2 parameters were **not** frozen in the original R4.7.41/R4.7.42 documentation drop. They are newly approved experimental design decisions, recorded here **before** workload source, HXF images, runner code, or Gate D1 results are produced.

No policy winner (RR vs FIFO) is predicted.

## 2. Frozen W2 parameters

| Parameter | Approved value |
|-----------|----------------|
| Processes | L, S1, S2 (3 total) |
| CPU workload | Exactly **12,000** pure-CPU loop iterations **per process** (L, S1, and S2 identical) |
| Creation sequence | L forks S1, then forks S2 |
| READY gate | L calls `sys_yield` after both forks |
| Completion | L performs WAIT twice, then exits with status **31** |
| Child exit codes | S1 exits **32**; S2 exits **33** |
| Step cap | **300,000** |
| Repetitions | **10** per policy |
| Profiles | Existing five profiles only (`balanced`, `fairness`, `latency`, `throughput`, `low_overhead`) |
| RR policy ID | **0** |
| FIFO policy ID | **1** |

## 3. Unchanged rules

- The five existing optimizer profiles in `src/opt_types.c` remain unchanged.
- Predeclared measured-outcome epsilon rules in the R4.7 specification remain unchanged.
- W2 is independent of R4.2 and must not modify the R4.2 catalog or evidence.
- `SCORE_EQUIVALENCE` does not imply `MEASURED_EQUIVALENCE`.
- Sensitivity `sens_*` profiles are out of scope for Gate D1.

## 4. Observability and calibration caveats

- Timer preemption **may** occur between the two fork calls. If available telemetry cannot establish that event, record **`NOT_OBSERVED`** (not `false`).
- PCB `ready_seq` is **not** claimed via the telemetry API.
- The 12,000-iteration CPU loop is a **workload mechanism**, not a calibrated wall-time or cross-machine instruction-count equivalence claim.

## 5. Approval timing

Approval occurred **before** implementation and execution of Gate D1 (W2).
