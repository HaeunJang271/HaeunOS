#include "haeun/planner.h"

#include "haeun/pval_validate.h"

#include <stdio.h>
#include <string.h>

/**
 * Runs R3.1 validation for recommended target (or source if KEEP).
 * Does not mutate guest RAM. Sets safety_* on plan.
 */
haeun_plan_result haeun_plan_safety_check(const haeun_vm *vm, haeun_plan *plan) {
    haeun_pval_report rep;
    uint32_t target;
    haeun_pval_result pr;
    if (vm == NULL || plan == NULL)
        return HAEUN_PLAN_BAD_ARGUMENT;
    plan->safety_ran = 0;
    plan->safety_preconditions_ok = 0;
    plan->safety_overall = HAEUN_PVAL_UNKNOWN;
    snprintf(plan->safety_status, sizeof(plan->safety_status), "not run");
    if (plan->decision == HAEUN_PLAN_NO_DECISION ||
        plan->decision == HAEUN_PLAN_KEEP_CURRENT) {
        snprintf(plan->safety_status, sizeof(plan->safety_status),
                 "skipped (no change requested)");
        return HAEUN_PLAN_OK;
    }
    target = plan->target_policy;
    pr = haeun_pval_validate(vm, target, &rep);
    plan->safety_ran = 1;
    if (pr != HAEUN_PVAL_OK) {
        plan->decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
        snprintf(plan->safety_status, sizeof(plan->safety_status),
                 "validation call failed");
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.1 validate error");
        return HAEUN_PLAN_REJECTED;
    }
    plan->safety_overall = rep.overall;
    if (rep.read_only != HAEUN_PVAL_PASS) {
        plan->decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
        snprintf(plan->safety_status, sizeof(plan->safety_status),
                 "read-only FAIL");
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.1 read-only audit failed");
        return HAEUN_PLAN_REJECTED;
    }
    if (!rep.compat_ran || !rep.compat.transition_preconditions_satisfied) {
        plan->decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
        plan->safety_preconditions_ok = 0;
        snprintf(plan->safety_status, sizeof(plan->safety_status),
                 "preconditions not satisfied (UNKNOWN≠PASS)");
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.1 transition preconditions failed or UNKNOWN "
                 "(RR→FIFO timestamp lifetime unbound if eligible exist)");
        return HAEUN_PLAN_REJECTED;
    }
    if (rep.compat.overall == HAEUN_PVAL_FAIL ||
        rep.compat.overall == HAEUN_PVAL_UNKNOWN) {
        plan->decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
        snprintf(plan->safety_status, sizeof(plan->safety_status),
                 "compat overall=%d", (int)rep.compat.overall);
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.1 FAIL/UNKNOWN blocks execution");
        return HAEUN_PLAN_REJECTED;
    }
    if (rep.invariants.overall == HAEUN_PVAL_FAIL ||
        rep.invariants.overall == HAEUN_PVAL_UNKNOWN) {
        plan->decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
        snprintf(plan->safety_status, sizeof(plan->safety_status),
                 "invariants overall=%d", (int)rep.invariants.overall);
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.1 invariants FAIL/UNKNOWN");
        return HAEUN_PLAN_REJECTED;
    }
    plan->safety_preconditions_ok = 1;
    snprintf(plan->safety_status, sizeof(plan->safety_status),
             "R3.1 preconditions PASS (WARN may remain in report)");
    return HAEUN_PLAN_OK;
}

/**
 * True if plan is still valid vs current monitor/policy/generation.
 */
int haeun_plan_is_valid(const haeun_plan *plan, const haeun_plan_obs *mon) {
    if (plan == NULL || mon == NULL)
        return 0;
    if (plan->snapshot_generation != mon->snapshot_generation)
        return 0;
    if (plan->source_policy != mon->policy_id)
        return 0;
    if (mon->timer_count > plan->expires_at_ticks)
        return 0;
    return 1;
}
