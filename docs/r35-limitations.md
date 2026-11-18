# R3.5 Limitations

## Implemented

- Host feedback metrics with explicit validity.
- Multi-objective before/after outcomes.
- SHADOW default; ADAPTIVE opt-in via CLI.
- State JSON save/load with schema check.
- Fixed RR / FIFO / Adaptive experiment harness on R2.2 workloads.

## Not claimed

- Adaptive superiority over fixed policies.
- Statistical significance of small-N experiments.
- ML / trained models.
- Wall-clock optimality.
- Full-VM rollback (R3.2 policy-ID only).
- SMP / multi-interpreter safety.

## Future work

- Runtime config file for objective weights (no recompile).
- Stronger CI intervals when reps are large.
- Tighter coupling of planner consecutive-improve with feedback counters.
