# R3.4 Limitations

## Implemented bounds

1. `ready_seq` safety assumes `wrap==0` (`2^64` READY entries). Wrap → UNKNOWN → reject.
2. Safe point is between `haeun_vm_step` calls in a single-threaded interpreter only. No SMP claim.
3. Host scheduler trace observes host-visible curr/dispatch deltas, not every internal `pick_next`.
4. Rollback restores the policy ID only — not full VM state.
5. `0xA474` is 0 until guest `sched_pol_init`; active tests must wait for live boot policy.

## Demonstrated (not universal)

- Active RR↔FIFO with `r12_pair` (eligible≥2) under seq+wrap0.
- Not claimed: every possible workload/transition is safe.

## Future work

- Stronger distinguishable RR-vs-FIFO selection workloads when both policies pick the same slot.
- Optional generation counters beyond seal for multi-observer hosts.
- Wrap handling beyond sticky UNKNOWN (epoch reboot / policy).
