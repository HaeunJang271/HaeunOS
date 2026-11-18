# R3.5 Preliminary Source Audit

Status: **Audit complete** (no kernel edits in this phase). Date: 2026-11-17.

## 1. Existing architecture

Pipeline today:

`telem → (R2.2 metrics) → R3.3 plan → R3.1 validate → R3.2 txn → R3.4 active sched/trace`

Host interpreter is single-threaded; safe point = between `haeun_vm_step`.

## 2. Available telemetry

PCB: `run_ticks`, `ready_wait_ticks`, `sleep_ticks`, `blocked_ticks`, `dispatch_count`,
`ready_since`, `ready_seq` lo/hi, complete_lo/valid. Globals: policy `0xA474`,
seq `0xA46C/A470`, wrap `0xA478`. Host: `haeun_telem_read`, `haeun_eval_metrics_from_*`.

## 3. Existing evaluation metrics

R2.2: aggregates, Jain fairness on `run_ticks`, cost score, stats (mean/median/min/max/stddev).
Optional metrics use `haeun_eval_opt.available` — zeros are not substituted for missing.

## 4. Workload definitions

Catalog (`eval_workload.c`): `cpu_bound`, `short_job`, `mixed`, `bursty`, `block_sleep`, `block_wait`.
Images under `build/` and `build/fcfs/`. Labels are catalog IDs, not runtime proof.

## 5. Policy transition

R3.2: request → prepare (R3.1) → commit policy word → optional rollback of **policy ID only**.
R3.4: active commit with eligible≥2 + `ready_seq` PASS when `wrap==0`.

## 6. Trace capabilities

`haeun_sched_trace_observe`: host-visible curr/dispatch/policy events. Not every internal pick.

## 7. R3.3 planner integration

`haeun_plan_run`: monitor → analyze → decide → safety → shadow (default) or `--execute` R3.2.
Stability: dwell, cooldown, consecutive recommends, oscillation guard. Decisions:
KEEP / RECOMMEND_CHANGE / NO_DECISION / REJECTED_BY_SAFETY.

## 8. R3.4 limitations (relevant to feedback)

- Seq safety only while wrap==0.
- No SMP; between-step safe point only.
- Trace is approximate; rollback ≠ full VM.
- `0xA474` zero until `sched_pol_init`.

## 9. Proposed feedback architecture (host-only)

```
Monitor → Analyzer → Planner(R3.3) → R3.1 → R3.2 → Trace(R3.4)
       → Feedback Evaluator → Decision History → Next cycle
```

New host modules: metrics validity view, before/after compare, multi-objective outcome,
controller state (save/load), CLI, experiment harness (fixed RR / fixed FIFO / Adaptive).

## 10. Expected source changes

- `include/haeun/feedback*.h`, `src/feedback_*.c`
- Minimal planner session hook for previous feedback outcome
- `src/main.c` CLI flags; Makefile `FEEDBACK_SRCS` + R35 tests
- `docs/r35-*.md`

## 11. Must remain unchanged

Kernel (hash prefix SHA-256, audit-time):

| File | Hash prefix |
|------|-------------|
| `proc_sched.s` | `F378293D24EF74FE` |
| `proc_policy.s` | `38A79C49D2F9F9D7` |
| `proc_trap.s` | `E6CB2A9823A5E6A4` |
| `proc_fork.s` | `D895E560F52A3361` |
| `proc_telem.s` | `2D7DBEACDB432683` |

No new ECALLs; no ABI change; no direct `0xA474` writes from feedback; R3.1 UNKNOWN≠PASS.

## Kernel necessity

Host-only is sufficient: metrics, compare, adapt, and execute already exist via R2.2–R3.4 APIs.
**No kernel modification for R3.5.**
