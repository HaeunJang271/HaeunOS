# R3.3 Safety Integration

R3.1 is authoritative. Planner does not reimplement invariants.

## Flow

On RECOMMEND_CHANGE: `haeun_pval_validate(vm, target)`.

- FAIL / UNKNOWN / unsatisfied preconditions → REJECTED_BY_SAFETY
- WARN is not reinterpreted as PASS
- PASS preconditions → may execute only if opt-in `--execute`

## RR → Preemptive FIFO

If READY/RUNNING eligible exist, timestamp lifetime `<2^31` remains unproven → UNKNOWN → **not executable**. Vacuous empty eligible may PASS; that does not prove active-workload reconfiguration.

Planner must not bypass, rewrite `ready_since`, or treat UNKNOWN as safe.
