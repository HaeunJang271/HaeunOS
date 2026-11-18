# R3.6 Experiment Design

Candidates: Fixed RR (`policy_id=0`), Fixed FIFO (`build/fcfs`, `policy_id=1`).

Workloads: cpu_bound, short_job, mixed, bursty, block_sleep, block_wait.

Each candidate×workload: `haeun_eval_experiment_run` with configurable reps (default 3).

Recorded: experiment id `r36-<kind>-<policy>`, success/fail, metric stats, status
VALID|INVALID|INSUFFICIENT|INCONCLUSIVE.

OPTIMIZE does not alter production scheduler policy. Adaptive (R3.5) remains a separate mode.
