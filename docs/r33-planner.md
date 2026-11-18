# R3.3 Planner

Deterministic rule-based planner (not ML).

## API

- `haeun_plan_monitor` / `analyze` / `decide` / `safety_check` / `run`
- `haeun_plan_fprint` / `haeun_plan_fprint_json` (`schema_version: 1`)
- CLI: `--plan-policy` [ `--plan-json` ] [ `--execute` ]

## Plan fields

`plan_id`, policies, decision, workload_class, evidence[], confidence, stability/safety status, shadow/txn flags.

## Decisions

KEEP_CURRENT | RECOMMEND_CHANGE | NO_DECISION | REJECTED_BY_SAFETY

## Evidence kinds

OK | ZERO | MISSING | INVALID | UNKNOWN | NA — missing is never treated as zero.
