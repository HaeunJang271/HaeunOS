#include "haeun/planner.h"

#include "haeun/reconfig.h"

#include <stdio.h>
#include <string.h>

/**
 * Full pipeline. shadow_mode=1 never commits. execute=1 only if shadow=0.
 */
haeun_plan_result haeun_plan_run(haeun_plan_session *sess, haeun_vm *vm,
                                 int shadow_mode, int execute, haeun_plan *out) {
    haeun_plan_obs mon;
    haeun_plan_result pr;
    uint32_t gen;
    if (sess == NULL || vm == NULL || out == NULL)
        return HAEUN_PLAN_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (execute && shadow_mode)
        shadow_mode = 1; /* shadow wins: never commit when shadow requested */
    if (execute && !shadow_mode)
        out->execute_requested = 1;
    out->shadow_mode = shadow_mode || !execute;
    sess->plan_seq++;
    gen = sess->plan_seq;
    snprintf(out->plan_id, sizeof(out->plan_id), "r33-%u", gen);
    pr = haeun_plan_monitor(vm, &mon, gen);
    if (pr != HAEUN_PLAN_OK) {
        out->decision = HAEUN_PLAN_NO_DECISION;
        snprintf(out->reason, sizeof(out->reason), "monitor failed");
        return pr;
    }
    pr = haeun_plan_analyze(&mon, &sess->cfg, out);
    if (pr != HAEUN_PLAN_OK && pr != HAEUN_PLAN_NO_TELEM) {
        out->decision = HAEUN_PLAN_NO_DECISION;
        return pr;
    }
    (void)haeun_plan_decide(sess, &mon, out);
    if (out->decision == HAEUN_PLAN_RECOMMEND_CHANGE) {
        pr = haeun_plan_safety_check(vm, out);
        (void)pr;
    } else {
        snprintf(out->safety_status, sizeof(out->safety_status),
                 "skipped (no change requested)");
    }
    if (out->shadow_mode) {
        /* Shadow: record expected outcome only; never touch policy word. */
        if (out->decision == HAEUN_PLAN_RECOMMEND_CHANGE &&
            out->safety_preconditions_ok)
            snprintf(out->rejection_reason, sizeof(out->rejection_reason),
                     "shadow: would attempt R3.2 (not executed)");
        out->txn_attempted = 0;
        out->txn_committed = 0;
        return HAEUN_PLAN_OK;
    }
    /* Opt-in execution path */
    if (out->decision != HAEUN_PLAN_RECOMMEND_CHANGE ||
        !out->safety_preconditions_ok) {
        out->txn_attempted = 0;
        return HAEUN_PLAN_OK;
    }
    if (!haeun_plan_is_valid(out, &mon)) {
        out->decision = HAEUN_PLAN_NO_DECISION;
        snprintf(out->rejection_reason, sizeof(out->rejection_reason),
                 "stale plan before execute");
        return HAEUN_PLAN_STALE;
    }
    {
        haeun_recfg_txn txn;
        haeun_recfg_result rr;
        haeun_recfg_fi_clear();
        out->txn_attempted = 1;
        rr = haeun_recfg_request(&txn, vm, out->source_policy, out->target_policy);
        if (rr != HAEUN_RECFG_OK) {
            snprintf(out->txn_id, sizeof(out->txn_id), "%s", txn.txn_id);
            snprintf(out->rejection_reason, sizeof(out->rejection_reason),
                     "R3.2 request rejected");
            return HAEUN_PLAN_REJECTED;
        }
        rr = haeun_recfg_prepare(&txn, vm);
        if (rr != HAEUN_RECFG_OK) {
            snprintf(out->txn_id, sizeof(out->txn_id), "%s", txn.txn_id);
            snprintf(out->rejection_reason, sizeof(out->rejection_reason),
                     "R3.2 prepare rejected");
            return HAEUN_PLAN_REJECTED;
        }
        rr = haeun_recfg_commit(&txn, vm);
        snprintf(out->txn_id, sizeof(out->txn_id), "%s", txn.txn_id);
        if (rr == HAEUN_RECFG_FAILED) {
            (void)haeun_recfg_rollback(&txn, vm);
            snprintf(out->rejection_reason, sizeof(out->rejection_reason),
                     "R3.2 commit failed; rollback state=%d", (int)txn.state);
            return HAEUN_PLAN_REJECTED;
        }
        if (rr != HAEUN_RECFG_OK || txn.state != HAEUN_RECFG_ST_COMMITTED) {
            snprintf(out->rejection_reason, sizeof(out->rejection_reason),
                     "R3.2 commit not committed");
            return HAEUN_PLAN_REJECTED;
        }
        out->txn_committed = 1;
    }
    return HAEUN_PLAN_OK;
}
