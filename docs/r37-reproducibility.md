# R3.7 Reproducibility

Effective seed = FNV-1a(workload_id || image || policy || step_cap || expected_min_pids).
Catalog workloads are intrinsically deterministic (no RNG).

Replay classes: EXACT_MATCH | METRIC_MATCH | DIVERGENT | INCOMPLETE | INVALID.

Config hash covers build_dir, preset, reps, min_success, profile_id.
