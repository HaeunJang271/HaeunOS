# R3.4 Scheduler Trace

Host `haeun_sched_trace_observe` records policy_id, selected slot, dispatch/context counters when curr/dispatch moves.

Does not alter guest scheduling. Behavioral evidence: post-commit events must show target `policy_id`.
