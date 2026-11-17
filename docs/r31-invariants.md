# R3.1 Invariants

| ID | Meaning |
|---|---|
| POL.recognized | Active ID is 0 (RR) or 1 (FIFO) |
| PCB.slot3_excluded | Slot 3 not a participant |
| PCB.state_range | AS slot states in 0..5 |
| CUR.single_running | ≤1 RUNNING |
| CUR.index_range | curr in 0..2 when not idle |
| CUR.running_match | curr vs RUNNING (WARN on exit edges) |
| CUR.idle_semantics | IDLE_FLAG set ⇒ RUNNING not required |
| TELEM.stale_free | WARN if FREE keeps counters |
| TELEM.complete_state | complete_valid only with ZOMBIE/FREE |

Statuses: PASS / FAIL / WARN / UNKNOWN / NOT_APPLICABLE.
