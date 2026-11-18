#ifndef HAEUN_PLANNER_H
#define HAEUN_PLANNER_H

#include "haeun/planner_types.h"
#include "haeun/vm.h"

#include <stdio.h>

/**
 * Collects telemetry, policy ID, and eval metrics. Read-only on guest RAM.
 */
haeun_plan_result haeun_plan_monitor(const haeun_vm *vm, haeun_plan_obs *out,
                                     uint32_t generation);

/**
 * Classifies workload from monitor evidence into wl_class + evidence[].
 */
haeun_plan_result haeun_plan_analyze(const haeun_plan_obs *mon,
                                     const haeun_plan_config *cfg,
                                     haeun_plan *plan);

/**
 * Applies deterministic rules + stability; fills decision fields.
 */
haeun_plan_result haeun_plan_decide(haeun_plan_session *sess,
                                    const haeun_plan_obs *mon,
                                    haeun_plan *plan);

/**
 * Runs R3.1 validation for recommended target (or source if KEEP).
 * Does not mutate guest RAM. Sets safety_* on plan.
 */
haeun_plan_result haeun_plan_safety_check(const haeun_vm *vm, haeun_plan *plan);

/**
 * Full pipeline. shadow_mode=1 never commits. execute=1 only if shadow=0.
 */
haeun_plan_result haeun_plan_run(haeun_plan_session *sess, haeun_vm *vm,
                                 int shadow_mode, int execute, haeun_plan *out);

/**
 * True if plan is still valid vs current monitor/policy/generation.
 */
int haeun_plan_is_valid(const haeun_plan *plan, const haeun_plan_obs *mon);

/**
 * Human-readable plan report.
 */
void haeun_plan_fprint(FILE *fp, const haeun_plan *plan);

/**
 * JSON plan report with schema_version.
 */
void haeun_plan_fprint_json(FILE *fp, const haeun_plan *plan);

/**
 * CLI entry: plan (shadow default). execute_opt-in commits via R3.2 only.
 * Returns 0 on success (plan produced), 1 on planner failure, 2 on bad args.
 */
int haeun_plan_cli_run(haeun_vm *vm, int json, int execute);

#endif
