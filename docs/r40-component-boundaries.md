# R4.0 Component Boundaries

## Guest kernel

- `kernel/process/proc_sched.s` — demote, pick_next, switch
- `kernel/process/proc_policy.s` — RR / preemptive FIFO select; `sched_pol_boot`
- `kernel/process/proc_trap.s` — timer/yield entry to scheduler
- `kernel/process/proc_telem.s` — counters; ready_seq take/assign
- `kernel/process/proc_fork.s` — child init including telem_child

## Host VM core

- `src/vm.c` — `haeun_vm_step` (single-threaded)
- `src/hxf.c` — image load

## Host research stack

| Package | Role |
|---------|------|
| `telem_inspect` | Read-only telemetry report |
| `eval_*` | Workload catalog + experiment runner |
| `pval_*` | Snapshot + invariants + timestamp/seq |
| `reconfig_*` | Transaction + FI + rollback (policy ID) |
| `planner_*` | Shadow/execute recommendations |
| `feedback_*` | Before/after feedback loop |
| `opt_*` | Multi-profile optimization |
| `robust_*` | Replay, sensitivity, stress, verify |
| `policy_verify` | Host mirror + sched trace |

## Forbidden crossings

- Planner/optimize/feedback/robust → direct `0xA474` write
- Treating host wall-clock as guest `run_ticks`
- Calling host tooling “in-kernel autonomy”
