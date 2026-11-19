# R4.7 W3 Parameter Freeze (R4.7.53)

**Status:** User-approved experimental design freeze for Gate D2.  
**Authority:** Explicit approval before W3 implementation and execution.  
**Relation:** Supplements [`docs/r47-recommendation-discrimination-spec.md`](r47-recommendation-discrimination-spec.md). Does not rewrite that document or W2 freeze/amendment files.

## 1. Purpose

W3 is an **I/O-interleaved** discrimination cell: CPU peers plus a real `sys_sleep` blocker, so sleep/blocked aggregates can be observed as **analysis-only** telemetry while the optimizer continues to score only the four established inputs.

No policy winner (RR vs FIFO) is predicted.

## 2. Frozen parameters (approved)

| Parameter | Frozen value |
|-----------|----------------|
| Workload | W3 I/O-interleaved |
| Processes | L, B, C (3 processes) |
| L role | CPU-bound parent |
| B role | SLEEP blocker |
| C role | CPU-bound peer |
| Creation order | L forks B, forks C, then `YIELD` |
| L CPU work | **6,000** pure-CPU loop iterations |
| C CPU work | **6,000** pure-CPU loop iterations |
| B blocking | `sys_sleep`, duration **64** (guest ticks; same interface as `r12_sleep`) |
| B completion | EXIT **42** after wake, without additional CPU workload |
| C completion | EXIT **43** after CPU workload |
| L completion | WAIT × 2, then EXIT **41** |
| Step cap | **800,000, fixed** |
| Repetitions | **10** per policy |
| Profiles | Existing five only |
| RR policy ID | **0** |
| FIFO policy ID | **1** |
| Execution mode | SHADOW only (no APPLY) |
| Optimizer inputs | `avg_ready_wait`, `jain_fairness`, `dispatch_count`, `completed` |
| Additional analysis metrics | sleep/blocked aggregates; available context-switch/timer telemetry |
| Predicted winner | **None** |

## 3. Process protocol

1. L starts and forks B.
2. L forks C (no intentional yield between forks).
3. L explicitly yields.
4. B invokes `sys_sleep(64)`.
5. B resumes after sleep and exits with code 42 (no extra CPU).
6. C performs exactly 6,000 pure-CPU iterations and exits with code 43.
7. L performs exactly 6,000 pure-CPU iterations, waits for both children, then exits with code 41.

Timer preemption between the two forks may occur; if not attributable via telemetry, record **`NOT_OBSERVED`**.

PCB `ready_seq` is **not** claimed via the telemetry API.

## 4. Strict step-cap rule

The confirmatory step cap is exactly **800,000**.

- Do not increase it automatically.
- Do not retry with a larger cap.
- Do not omit or discard a step-cap failure.
- If any run exceeds 800,000 steps, **stop Gate D2 immediately**, preserve evidence, and require separate user approval before any amendment.

A successful diagnostic at a larger cap does **not** authorize a larger confirmatory cap.

## 5. Scoring vs analysis

- **Optimizer inputs only:** ready, jain, dispatch, completed (existing profiles/weights/ε unchanged).
- **Analysis-only:** `total_sleep`, `total_blocked`, and available `context_switches` / `timer_irqs`.
- `SCORE_EQUIVALENCE` ≠ `MEASURED_EQUIVALENCE`.

## 6. Isolation

W3 is independent of R4.2 and must not modify the catalog, R4.2 evidence, Gate B/C/D1 artifacts, production optimizer profiles, or `v4.6.0`.
