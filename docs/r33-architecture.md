# R3.3 Architecture

Pipeline (host-side):

Monitor → Analyzer → Planner → Safety Validator (R3.1) → Transaction Executor (R3.2, opt-in)

Default mode: **SHADOW** (plan + safety probe; no guest policy write).

## Components

| Stage | Module | Mutates guest? |
|---|---|---|
| Monitor | `planner_monitor.c` | No |
| Analyzer | `planner_analyze.c` | No |
| Decide | `planner_decide.c` | No |
| Safety | `planner_safety.c` → `haeun_pval_validate` | No |
| Execute | `planner_run.c` → `haeun_recfg_*` | Only if `--execute` and safety PASS |

## Not implemented

- ML / trained models
- New ECALLs
- Kernel scheduler changes
- Direct `0xA474` writes in planner modules

## Claim accuracy

R3.3 implements autonomous **recommendation** with evidence and gates. Autonomous **kernel commit** is not the default and occurs only via R3.2 when explicitly enabled.
