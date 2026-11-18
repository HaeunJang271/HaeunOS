# R4.0 Research Hypotheses

**Primary question:** Can an OS use a *host-assisted* autonomous optimization framework to evaluate, select, validate, and safely apply runtime scheduling policies while preserving explicitly defined kernel invariants?

Terminology: “host-assisted AROS,” not “fully autonomous production OS.”

## H1 — Policy abstraction

**Statement:** The active scheduler supports RR and preemptive FIFO via an explicit policy-selection abstraction without changing core dispatch structure.

| Item | Definition |
|------|------------|
| Variables | `policy_id` at `0xA474`; selected PCB; dispatch path |
| Evidence | `proc_policy.s`, r20/r30 tests |
| Pass | Both policies select eligible PCBs; invalid ID fails closed |
| Fail | Unintended silent fallback or corrupted PCB |
| Confounders | Workload HXF differences between RR/FIFO image trees |
| Limits | Two policies only; single-threaded |

## H2 — Runtime reconfiguration safety

**Statement:** Policy changes occur via a validated transaction at a defined between-step safe point under single-threaded assumptions.

| Item | Definition |
|------|------------|
| Variables | txn state; pval verdict; post-commit policy word |
| Evidence | r31–r34 guest tests; R3.2 rollback path |
| Pass | PASS→commit; UNKNOWN/FAIL≠commit; rollback restores prior ID |
| Fail | Commit after UNKNOWN; success reported after failed write |
| Confounders | Active-process mid-quantum; wrap sticky flag |
| Limits | No SMP; no HW atomicity; rollback = policy ID only |

## H3 — Autonomous recommendation

**Statement:** Host optimization evaluates candidates across workloads/profiles and emits explicit recommendations.

| Item | Definition |
|------|------------|
| Variables | profile scores; Pareto set; recommended policy |
| Evidence | r36 unit/guest; opt CLI |
| Pass | Five profiles produce typed recommendations without APPLY |
| Fail | Recommendation writes `0xA474` without txn |
| Confounders | Metric validity flags; missing samples |
| Limits | Fixed catalog; host-side only |

## H4 — Recommendation reliability

**Statement:** Repeated runs and sensitivity analysis identify conditions where recommendations are stable/reproducible.

| Item | Definition |
|------|------------|
| Variables | replay class; sensitivity class; verify outcome |
| Evidence | R3.7/R4.0 smoke (`reps=3`) |
| Pass | Smoke: EXACT/METRIC match; non-FAIL overall |
| Insufficient | Small-N; no inferential superiority claim |
| Confounders | Host wall-clock noise (ignored in fingerprints) |
| Limits | Research/100-rep NOT auto-run |

## H5 — Safety separation

**Statement:** Optimization, recommendation, safety validation, and application remain separate operations.

| Item | Definition |
|------|------------|
| Variables | SHADOW vs APPLY; pval gate before commit |
| Evidence | feedback/opt/reconfig tests |
| Pass | SHADOW never commits; APPLY requires explicit path + PASS |
| Fail | Optimizer mutates kernel policy memory directly |
| Confounders | CLI flags misinterpreted as default APPLY |
| Limits | Operator discipline for ADAPTIVE mode |

**Rule:** Feature existence alone does not mark a hypothesis “supported.”
