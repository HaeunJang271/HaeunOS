#include "haeun/optimize.h"

#include "haeun/planner.h"
#include "haeun/reconfig.h"

#include <stdio.h>
#include <string.h>

/**
 * Applies a recommendation via R3.1→R3.2 only (never writes policy directly).
 */
haeun_opt_result haeun_opt_apply(haeun_vm *vm,
                                 const haeun_opt_recommendation *rec,
                                 haeun_plan *plan_out) {
    haeun_recfg_txn txn;
    haeun_recfg_result rr;
    haeun_plan plan;
    uint32_t cur;
    if (vm == NULL || rec == NULL || !rec->valid)
        return HAEUN_OPT_BAD_ARGUMENT;
    if (rec->schema_version != HAEUN_OPT_SCHEMA)
        return HAEUN_OPT_BAD_STATE;
    if (rec->decision != HAEUN_OPT_DEC_SELECT)
        return HAEUN_OPT_REJECTED;
    memset(&plan, 0, sizeof(plan));
    plan.source_policy = 0u;
    if (vm->ram != NULL && vm->ram_size > 0xA478u) {
        cur = (uint32_t)vm->ram[0xA474] | ((uint32_t)vm->ram[0xA475] << 8) |
              ((uint32_t)vm->ram[0xA476] << 16) |
              ((uint32_t)vm->ram[0xA477] << 24);
        plan.source_policy = cur;
    }
    plan.target_policy = rec->recommended_policy;
    plan.decision = HAEUN_PLAN_RECOMMEND_CHANGE;
    if (plan.source_policy == plan.target_policy) {
        plan.decision = HAEUN_PLAN_KEEP_CURRENT;
        snprintf(plan.reason, sizeof(plan.reason), "already at recommended policy");
        if (plan_out != NULL)
            *plan_out = plan;
        return HAEUN_OPT_OK;
    }
    if (haeun_plan_safety_check(vm, &plan) != HAEUN_PLAN_OK ||
        !plan.safety_preconditions_ok) {
        plan.decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
        if (plan_out != NULL)
            *plan_out = plan;
        return HAEUN_OPT_REJECTED;
    }
    haeun_recfg_fi_clear();
    plan.txn_attempted = 1;
    rr = haeun_recfg_request(&txn, vm, plan.source_policy, plan.target_policy);
    if (rr != HAEUN_RECFG_OK) {
        if (plan_out != NULL)
            *plan_out = plan;
        return HAEUN_OPT_REJECTED;
    }
    rr = haeun_recfg_prepare(&txn, vm);
    if (rr != HAEUN_RECFG_OK) {
        if (plan_out != NULL)
            *plan_out = plan;
        return HAEUN_OPT_REJECTED;
    }
    rr = haeun_recfg_commit(&txn, vm);
    snprintf(plan.txn_id, sizeof(plan.txn_id), "%s", txn.txn_id);
    if (rr == HAEUN_RECFG_FAILED) {
        (void)haeun_recfg_rollback(&txn, vm);
        if (plan_out != NULL)
            *plan_out = plan;
        return HAEUN_OPT_REJECTED;
    }
    if (rr != HAEUN_RECFG_OK || txn.state != HAEUN_RECFG_ST_COMMITTED) {
        if (plan_out != NULL)
            *plan_out = plan;
        return HAEUN_OPT_REJECTED;
    }
    plan.txn_committed = 1;
    if (plan_out != NULL)
        *plan_out = plan;
    return HAEUN_OPT_OK;
}
