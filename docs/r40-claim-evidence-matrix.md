# R4.0 Claim-to-Evidence Matrix

| ID | Claim | Implementation | Test | Experiment | Status | Limitations |
|----|-------|----------------|------|------------|--------|-------------|
| C1 | Scheduler supports RR and preemptive FIFO | `proc_policy.s` | r20/r30 | catalog | VERIFIED | 2 policies only |
| C2 | Host optimization evaluates candidates | `opt_*.c` | r36 | smoke | VERIFIED | fixed HXF |
| C3 | Recommendation ≠ automatic apply | planner/opt/feedback CLI | r33/r35/r36 | — | VERIFIED | SHADOW default |
| C4 | Policy change is validation-gated txn | `reconfig_txn.c`+pval | r32/r34 | active guest | VERIFIED | single-thread SP |
| C5 | Same config replayable | `robust_replay.c` | r37/r40/r41 | R4.1 smoke+standard EXACT_MATCH×6 | OBSERVED | deterministic images; n=3/10 |
| C6 | Sensitivity evaluates stability | `robust_sensitivity.c` | r37/r40/r41 | R4.1 smoke+standard STABLE×6 | OBSERVED | small perturbations |
| C7 | Transitions preserve documented invariants under **tested** conditions | pval+reconfig | r31–r34/r37 | active/stress | OBSERVED | not universal proof; r40 stress dry |
| C8 | Feedback SHADOW never commits | `feedback_controller.c` | r35 | guest | VERIFIED | — |
| C9 | ready_seq enables FIFO safety when wrap==0 | telem+pval | r34 | active RR→FIFO | VERIFIED | wrap→UNKNOWN |
| C10 | Inferential superiority of Adaptive/Opt | — | — | — | INSUFFICIENT | not claimed |

## Hypotheses mapping

| H | Related claims | Support |
|---|----------------|---------|
| H1 Policy abstraction | C1 | Supported under catalog |
| H2 Reconfig safety | C4,C7,C9 | Supported under tested SP assumptions |
| H3 Autonomous recommendation | C2,C3 | Supported as host-assisted |
| H4 Reliability | C5,C6 | Supported at smoke scale only |
| H5 Safety separation | C3,C4,C8 | Supported |
