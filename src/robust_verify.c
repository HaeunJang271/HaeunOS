#include "haeun/robust.h"

#include <stdio.h>
#include <string.h>

/**
 * Verify R3.6-style recommendations against R3.7 evidence.
 * Does not rewrite historical recommendations; fills verify fields.
 */
haeun_rb_result haeun_rb_verify_recommendations(haeun_rb_state *st) {
    unsigned w;
    if (st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    for (w = 0; w < st->wl_count; ++w) {
        haeun_rb_wl_report *wr = &st->workloads[w];
        wr->verified_policy = wr->original_policy;
        if (wr->reliability == HAEUN_RB_REL_INVALID ||
            wr->replay == HAEUN_RB_INVALID) {
            wr->verify = HAEUN_RB_VERIFY_INVALID;
        } else if (wr->reliability == HAEUN_RB_REL_INSUFFICIENT ||
                   wr->reliability == HAEUN_RB_REL_INCONCLUSIVE ||
                   wr->replay == HAEUN_RB_INCOMPLETE) {
            wr->verify = HAEUN_RB_VERIFY_INSUFFICIENT;
        } else if (wr->original_decision == HAEUN_OPT_DEC_INCONCLUSIVE ||
                   wr->original_decision == HAEUN_OPT_DEC_NO_DECISION) {
            wr->verify = HAEUN_RB_VERIFY_INSUFFICIENT;
        } else if (wr->original_decision == HAEUN_OPT_DEC_EQUIVALENT) {
            wr->verify = HAEUN_RB_VERIFY_CONDITIONAL;
            wr->verified_policy = wr->original_policy;
        } else if (wr->original_decision == HAEUN_OPT_DEC_SELECT) {
            if (wr->sensitivity == HAEUN_RB_UNSTABLE)
                wr->verify = HAEUN_RB_VERIFY_NOT_CONFIRMED;
            else if (wr->sensitivity == HAEUN_RB_SENSITIVE)
                wr->verify = HAEUN_RB_VERIFY_CONDITIONAL;
            else if (wr->replay == HAEUN_RB_EXACT_MATCH ||
                     wr->replay == HAEUN_RB_METRIC_MATCH)
                wr->verify = HAEUN_RB_VERIFY_CONFIRMED;
            else
                wr->verify = HAEUN_RB_VERIFY_CONDITIONAL;
        } else {
            wr->verify = HAEUN_RB_VERIFY_INSUFFICIENT;
        }
    }
    return HAEUN_RB_OK;
}

/**
 * Aggregate overall PASS/WARN/UNKNOWN/FAIL (conservative).
 */
void haeun_rb_aggregate(haeun_rb_state *st) {
    unsigned w;
    int any_fail = 0, any_unknown = 0, any_warn = 0;
    if (st == NULL)
        return;
    if (st->stress_failures > st->stress_transitions &&
        st->stress_transitions > 0u)
        any_warn = 1;
    for (w = 0; w < st->wl_count; ++w) {
        const haeun_rb_wl_report *wr = &st->workloads[w];
        if (wr->verify == HAEUN_RB_VERIFY_INVALID ||
            wr->reliability == HAEUN_RB_REL_INVALID ||
            wr->replay == HAEUN_RB_DIVERGENT)
            any_fail = 1;
        else if (wr->verify == HAEUN_RB_VERIFY_INSUFFICIENT ||
                 wr->reliability == HAEUN_RB_REL_INSUFFICIENT)
            any_unknown = 1;
        else if (wr->verify == HAEUN_RB_VERIFY_NOT_CONFIRMED ||
                 wr->verify == HAEUN_RB_VERIFY_CONDITIONAL ||
                 wr->sensitivity == HAEUN_RB_SENSITIVE)
            any_warn = 1;
    }
    if (any_fail) {
        st->overall = HAEUN_RB_OVERALL_FAIL;
        snprintf(st->overall_reason, sizeof(st->overall_reason),
                 "invalid or divergent evidence present");
    } else if (any_unknown) {
        st->overall = HAEUN_RB_OVERALL_UNKNOWN;
        snprintf(st->overall_reason, sizeof(st->overall_reason),
                 "insufficient evidence on one or more workloads");
    } else if (any_warn) {
        st->overall = HAEUN_RB_OVERALL_WARN;
        snprintf(st->overall_reason, sizeof(st->overall_reason),
                 "conditional/sensitive results present");
    } else {
        st->overall = HAEUN_RB_OVERALL_PASS;
        snprintf(st->overall_reason, sizeof(st->overall_reason),
                 "reproducible sufficient evidence under preset");
    }
}
