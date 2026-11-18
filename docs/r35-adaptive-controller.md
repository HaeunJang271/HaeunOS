# R3.5 Adaptive Controller

## Cycle (`haeun_fb_cycle`)

1. Sample baseline from live VM.
2. Run R3.3 plan in shadow.
3. Apply feedback gates (`haeun_fb_apply_to_plan`).
4. If ADAPTIVE+execute+RECOMMEND+safety: fresh R3.1 then R3.2 commit.
5. Sample after; evaluate only if committed.
6. Record history / cooldown / consecutive counters.

## Stability (extends R3.3)

| Control | Default |
|---------|---------|
| cooldown_cycles | 1 after DEGRADED |
| consecutive_improve_needed | 2 (config; informational) |
| consecutive_regress_needed | 2 |
| hysteresis | via gates + planner dwell |
| duplicate suppression | planner history |

Identical inputs + state → identical decisions.

## Objective defaults (experimental)

| Metric | Dir | Weight | Improve / Regress Δ |
|--------|-----|--------|---------------------|
| avg_ready_wait | lower | 0.40 | 5% / 5% |
| jain_fairness | higher | 0.25 | 2% / 5% |
| dispatch_count | lower | 0.20 | 10% / 15% |
| completed | higher | 0.15 | any / any |

Not proven optimal. Configurable in `haeun_fb_config` (recompile today).
