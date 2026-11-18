# R3.3 Pre-Implementation Audit

## 1. Current architecture

Host C17 interpreter (`haeun_vm_step`) runs guest HAEUN-32. Scheduler policy ID at guest `0xA474` (RR=0, Preemptive FIFO/`FCFS`=1). Selection in `policy_select` → `policy_rr` / `policy_fcfs` (`ready_since` modular order). Telemetry in PCB + globals (`0xA460`…). Host tools: telem inspector (R1.3), eval (R2.2), pval (R3.1), reconfig txn (R3.2).

## 2. Scheduler policy interfaces

| Item | Source |
|---|---|
| Policy word | `0xA474` |
| Boot default | `sched_pol_boot` → RR |
| Select | `policy_select` in `proc_policy.s` |
| FCFS meaning | Preemptive FIFO by `ready_since`, not classical non-preemptive FCFS |

## 3. Available telemetry (host-readable)

Per AS slot (0..2): `run_ticks`, `ready_wait_ticks`, `sleep_ticks`, `blocked_ticks`, `dispatch_count`, `ready_since`, state/pid, completion fields. Globals: `idle_flag`, `timer_irqs`, `context_switches`, `curr_index`. API: `haeun_telem_read`. Missing RAM → error; FREE may retain stale counters (EXPECTED note) — must not treat as live evidence without checks.

## 4. R2.2 evaluation

Workload kinds: CPU_BOUND, SHORT_JOB, MIXED, BURSTY, BLOCK_SLEEP, BLOCK_WAIT. Metrics use `haeun_eval_opt` (available flag; never coerce missing→0). Guest ticks ≠ wall clock. Jain fairness ≠ overall quality.

## 5. R3.1 validation

`haeun_pval_validate(vm, target, &report)`. PASS/WARN/UNKNOWN/FAIL. RR→FIFO needs timestamp gates; lifetime `<2^31` unbound → often UNKNOWN with READY/RUNNING. Vacuous empty eligible may PASS. WARN ≠ PASS.

## 6. R3.2 transaction

`haeun_recfg_request/prepare/commit/rollback`. Sole writer of policy word. Serialized between steps. Rollback = policy ID only.

## 7. Limitations relevant to R3.3

- No PCB epoch → RR→FIFO often non-executable under R3.1.
- Single-threaded host VM; no SMP claim.
- Telemetry is accounting, not throughput proof.
- Small eval reps ≠ statistical significance.

## 8. Proposed R3.3 integration

Host-only pipeline: Monitor → Analyzer → Planner → Safety(R3.1) → (optional) Executor(R3.2). Default **SHADOW**: plan + safety probe, no commit. Optional `--plan-policy --execute` opt-in.

## 9. Expected changes

New: `include/haeun/planner*.h`, `src/planner_*.c`, `tests/r33_*.c`, `docs/r33-*.md`, Makefile, `main.c` CLI flags.

## 10. Must remain unchanged

`proc_sched.s`, `proc_policy.s`, `proc_trap.s`, `proc_telem.s` (hashes at audit):

| File | SHA-1 |
|---|---|
| `proc_sched.s` | `fe6a0ef11864a1947e5fc28d3929e86bd0d55233` |
| `proc_policy.s` | `dee3a76622a8d2acfcb5a0e958b14f74056dbb52` |
| `proc_trap.s` | `a2c1f917b346bcb42ed884d102065cf006091a89` |
| `proc_telem.s` | `8349bdacf7019e239941fc4a6f39c40088b637c4` |

No new ECALLs. No direct `0xA474` writes in planner code. Do not weaken R3.1/R3.2.
