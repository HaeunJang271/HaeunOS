#include "haeun/feedback.h"

#include "haeun/planner.h"
#include "haeun/reconfig.h"

#include <stdio.h>
#include <string.h>

/**
 * Applies feedback gates to a planner recommendation (does not execute).
 */
void haeun_fb_apply_to_plan(const haeun_fb_state *st, haeun_plan *plan) {
    if (st == NULL || plan == NULL)
        return;
    if (plan->decision != HAEUN_PLAN_RECOMMEND_CHANGE)
        return;
    if (st->cooldown_remaining > 0u) {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->target_policy = plan->source_policy;
        snprintf(plan->reason, sizeof(plan->reason),
                 "feedback cooldown (%u remaining)", st->cooldown_remaining);
        snprintf(plan->stability_status, sizeof(plan->stability_status),
                 "feedback:cooldown");
        return;
    }
    if (!st->previous_outcome_valid)
        return;
    if (st->previous_outcome == HAEUN_FB_INCONCLUSIVE) {
        plan->decision = HAEUN_PLAN_NO_DECISION;
        plan->target_policy = plan->source_policy;
        snprintf(plan->reason, sizeof(plan->reason),
                 "previous feedback inconclusive; retain policy");
        return;
    }
    if (st->previous_outcome == HAEUN_FB_DEGRADED &&
        st->consecutive_regressions >= st->cfg.consecutive_regress_needed &&
        plan->target_policy == st->previous_policy) {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->target_policy = plan->source_policy;
        snprintf(plan->reason, sizeof(plan->reason),
                 "feedback: avoid re-entering degraded policy");
        return;
    }
}

static int exec_txn(haeun_plan *plan, haeun_vm *vm) {
    haeun_recfg_txn txn;
    haeun_recfg_result rr;
    haeun_recfg_fi_clear();
    plan->txn_attempted = 1;
    rr = haeun_recfg_request(&txn, vm, plan->source_policy, plan->target_policy);
    if (rr != HAEUN_RECFG_OK) {
        snprintf(plan->txn_id, sizeof(plan->txn_id), "%s", txn.txn_id);
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.2 request rejected");
        return 0;
    }
    rr = haeun_recfg_prepare(&txn, vm);
    if (rr != HAEUN_RECFG_OK) {
        snprintf(plan->txn_id, sizeof(plan->txn_id), "%s", txn.txn_id);
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.2 prepare rejected");
        return 0;
    }
    rr = haeun_recfg_commit(&txn, vm);
    snprintf(plan->txn_id, sizeof(plan->txn_id), "%s", txn.txn_id);
    if (rr == HAEUN_RECFG_FAILED) {
        (void)haeun_recfg_rollback(&txn, vm);
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.2 commit failed; rollback state=%d", (int)txn.state);
        return 0;
    }
    if (rr != HAEUN_RECFG_OK || txn.state != HAEUN_RECFG_ST_COMMITTED) {
        snprintf(plan->rejection_reason, sizeof(plan->rejection_reason),
                 "R3.2 commit not committed");
        return 0;
    }
    plan->txn_committed = 1;
    return 1;
}

/**
 * Records assessment into controller history/stability counters.
 */
void haeun_fb_record_outcome(haeun_fb_state *st, const haeun_plan *plan,
                             const haeun_fb_assessment *a, int txn_committed) {
    haeun_fb_hist_entry *e;
    if (st == NULL || plan == NULL || a == NULL)
        return;
    st->previous_decision = plan->decision;
    st->previous_outcome = a->outcome;
    st->previous_outcome_valid = 1;
    st->previous_policy = st->current_policy;
    if (txn_committed)
        st->current_policy = plan->target_policy;
    if (a->outcome == HAEUN_FB_IMPROVED) {
        st->consecutive_improvements++;
        st->consecutive_regressions = 0u;
    } else if (a->outcome == HAEUN_FB_DEGRADED) {
        st->consecutive_regressions++;
        st->consecutive_improvements = 0u;
        st->cooldown_remaining = st->cfg.cooldown_cycles;
    } else {
        st->consecutive_improvements = 0u;
        st->consecutive_regressions = 0u;
    }
    if (st->hist_count < HAEUN_FB_HIST_MAX)
        e = &st->history[st->hist_count++];
    else {
        unsigned i;
        for (i = 1; i < HAEUN_FB_HIST_MAX; ++i)
            st->history[i - 1u] = st->history[i];
        e = &st->history[HAEUN_FB_HIST_MAX - 1u];
    }
    memset(e, 0, sizeof(*e));
    e->cycle_id = st->cycle_id;
    e->from_policy = plan->source_policy;
    e->to_policy = plan->target_policy;
    e->decision = plan->decision;
    e->outcome = a->outcome;
    e->txn_committed = txn_committed;
    snprintf(e->reason, sizeof(e->reason), "%s", a->reason);
}

/**
 * One adaptive cycle. Shadow/dry never commits; adaptive+execute may via R3.2.
 */
haeun_fb_result haeun_fb_cycle(haeun_fb_state *st, haeun_plan_session *psess,
                               haeun_vm *vm, int execute, haeun_plan *plan_out,
                               haeun_fb_assessment *assess_out) {
    haeun_plan plan;
    haeun_fb_sample before;
    haeun_fb_sample after;
    haeun_fb_assessment assess;
    haeun_plan_result pr;
    int want_exec;
    int committed = 0;
    if (st == NULL || psess == NULL || vm == NULL || plan_out == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    st->cycle_id++;
    if (st->cooldown_remaining > 0u)
        st->cooldown_remaining--;
    st->measurement_generation++;
    if (haeun_fb_sample_from_vm(vm, "live", st->workload_fingerprint,
                                &before) != HAEUN_FB_OK)
        return HAEUN_FB_BAD_ARGUMENT;
    st->baseline = before;
    st->current_policy = before.policy_id;
    /* Always plan in shadow first so feedback can gate before any commit. */
    pr = haeun_plan_run(psess, vm, 1, 0, &plan);
    if (pr != HAEUN_PLAN_OK && pr != HAEUN_PLAN_NO_TELEM) {
        *plan_out = plan;
        return HAEUN_FB_REJECTED;
    }
    haeun_fb_apply_to_plan(st, &plan);
    want_exec = execute && st->mode == HAEUN_FB_MODE_ADAPTIVE &&
                plan.decision == HAEUN_PLAN_RECOMMEND_CHANGE &&
                plan.safety_preconditions_ok;
    plan.shadow_mode = want_exec ? 0 : 1;
    plan.execute_requested = want_exec ? 1 : 0;
    if (want_exec) {
        /* Fresh R3.1 gate immediately before R3.2 (plan may be stale). */
        (void)haeun_plan_safety_check(vm, &plan);
        if (plan.decision == HAEUN_PLAN_RECOMMEND_CHANGE &&
            plan.safety_preconditions_ok)
            committed = exec_txn(&plan, vm);
        else if (plan.decision != HAEUN_PLAN_REJECTED_BY_SAFETY) {
            plan.decision = HAEUN_PLAN_REJECTED_BY_SAFETY;
            snprintf(plan.rejection_reason, sizeof(plan.rejection_reason),
                     "safety blocked adaptive execute");
        }
    }
    if (haeun_fb_sample_from_vm(vm, "live", st->workload_fingerprint,
                                &after) != HAEUN_FB_OK)
        return HAEUN_FB_BAD_ARGUMENT;
    st->post = after;
    if (committed)
        (void)haeun_fb_evaluate(&st->cfg, &before, &after, &assess);
    else {
        memset(&assess, 0, sizeof(assess));
        assess.outcome = HAEUN_FB_INCONCLUSIVE;
        snprintf(assess.reason, sizeof(assess.reason),
                 "no committed transition this cycle");
        snprintf(assess.limitations, sizeof(assess.limitations),
                 "assessment requires committed change");
    }
    haeun_fb_record_outcome(st, &plan, &assess, committed);
    *plan_out = plan;
    if (assess_out != NULL)
        *assess_out = assess;
    return HAEUN_FB_OK;
}
