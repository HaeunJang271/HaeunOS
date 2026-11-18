#include "haeun/planner.h"

#include <stdio.h>
#include <string.h>

static double find_ev(const haeun_plan *plan, const char *name, int *ok) {
    unsigned i;
    *ok = 0;
    for (i = 0; i < plan->evidence_count; ++i) {
        if (strcmp(plan->evidence[i].name, name) == 0) {
            if (plan->evidence[i].kind == HAEUN_PLAN_EV_OK ||
                plan->evidence[i].kind == HAEUN_PLAN_EV_ZERO) {
                *ok = 1;
                return plan->evidence[i].value;
            }
            return 0.0;
        }
    }
    return 0.0;
}

static void push_hist(haeun_plan_session *sess, haeun_plan_decision d,
                      uint32_t tgt) {
    if (sess->hist_count < HAEUN_PLAN_HIST_MAX) {
        sess->hist_dec[sess->hist_count] = d;
        sess->hist_tgt[sess->hist_count] = tgt;
        sess->hist_count++;
    } else {
        unsigned i;
        for (i = 1; i < HAEUN_PLAN_HIST_MAX; ++i) {
            sess->hist_dec[i - 1u] = sess->hist_dec[i];
            sess->hist_tgt[i - 1u] = sess->hist_tgt[i];
        }
        sess->hist_dec[HAEUN_PLAN_HIST_MAX - 1u] = d;
        sess->hist_tgt[HAEUN_PLAN_HIST_MAX - 1u] = tgt;
    }
}

static int stability_suppress(haeun_plan_session *sess,
                              const haeun_plan_obs *mon, uint32_t tgt,
                              char *why, size_t why_n) {
    uint64_t since;
    if (sess->last_decision_ticks != 0u) {
        since = mon->timer_count - sess->last_decision_ticks;
        if (since < (uint64_t)sess->cfg.min_dwell_ticks) {
            snprintf(why, why_n, "suppressed: min_dwell (%llu < %u)",
                     (unsigned long long)since, sess->cfg.min_dwell_ticks);
            return 1;
        }
        if (since < (uint64_t)sess->cfg.cooldown_ticks) {
            snprintf(why, why_n, "suppressed: cooldown (%llu < %u)",
                     (unsigned long long)since, sess->cfg.cooldown_ticks);
            return 1;
        }
    }
    if (sess->last_recommend_target == tgt &&
        sess->consecutive_same + 1u < sess->cfg.consecutive_needed) {
        snprintf(why, why_n, "suppressed: need %u consecutive recommends",
                 sess->cfg.consecutive_needed);
        return 1;
    }
    if (sess->hist_count >= 2u) {
        unsigned h = sess->hist_count;
        if (sess->hist_dec[h - 1u] == HAEUN_PLAN_RECOMMEND_CHANGE &&
            sess->hist_dec[h - 2u] == HAEUN_PLAN_RECOMMEND_CHANGE &&
            sess->hist_tgt[h - 1u] != sess->hist_tgt[h - 2u] &&
            sess->hist_tgt[h - 1u] == mon->policy_id) {
            snprintf(why, why_n, "suppressed: oscillation guard");
            return 1;
        }
    }
    snprintf(why, why_n, "ok");
    return 0;
}

/**
 * Applies deterministic rules + stability; fills decision fields.
 */
haeun_plan_result haeun_plan_decide(haeun_plan_session *sess,
                                    const haeun_plan_obs *mon,
                                    haeun_plan *plan) {
    uint32_t want;
    int ok = 0;
    double ready_run, sleep_share;
    char stab[HAEUN_PLAN_MSG_LEN];
    if (sess == NULL || mon == NULL || plan == NULL)
        return HAEUN_PLAN_BAD_ARGUMENT;
    plan->source_policy = mon->policy_id;
    plan->target_policy = mon->policy_id;
    plan->created_at_ticks = mon->timer_count;
    plan->expires_at_ticks = mon->timer_count + sess->cfg.plan_ttl_ticks;
    plan->snapshot_generation = mon->snapshot_generation;
    plan->confidence = HAEUN_PLAN_CONF_NONE;
    plan->decision = HAEUN_PLAN_NO_DECISION;
    snprintf(plan->reason, sizeof(plan->reason), "insufficient evidence");
    snprintf(plan->stability_status, sizeof(plan->stability_status), "n/a");
    if (!mon->telem_ok || !mon->policy_ok) {
        plan->decision = HAEUN_PLAN_NO_DECISION;
        snprintf(plan->reason, sizeof(plan->reason),
                 "missing or invalid monitor inputs");
        push_hist(sess, plan->decision, plan->target_policy);
        return HAEUN_PLAN_OK;
    }
    if (plan->workload_class == HAEUN_PLAN_WL_UNKNOWN) {
        plan->decision = HAEUN_PLAN_NO_DECISION;
        snprintf(plan->reason, sizeof(plan->reason),
                 "workload classification UNKNOWN");
        push_hist(sess, plan->decision, plan->target_policy);
        return HAEUN_PLAN_OK;
    }
    if (plan->workload_class == HAEUN_PLAN_WL_IDLE) {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->confidence = HAEUN_PLAN_CONF_LOW;
        snprintf(plan->reason, sizeof(plan->reason),
                 "idle/vacuous activity; keep current");
        push_hist(sess, plan->decision, plan->target_policy);
        sess->last_decision_ticks = mon->timer_count;
        sess->last_policy = mon->policy_id;
        return HAEUN_PLAN_OK;
    }
    want = mon->policy_id;
    ready_run = find_ev(plan, "ready_run_ratio", &ok);
    sleep_share = find_ev(plan, "sleep_block_share", &ok);
    (void)sleep_share;
    if (!sess->cfg.experimental_rules) {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->confidence = HAEUN_PLAN_CONF_LOW;
        snprintf(plan->reason, sizeof(plan->reason),
                 "experimental rules disabled; keep current");
    } else if (plan->workload_class == HAEUN_PLAN_WL_SHORT_JOB &&
               mon->policy_id == HAEUN_EVAL_POLICY_RR) {
        /* Experimental: short jobs may benefit from ready_since FIFO order. */
        want = HAEUN_EVAL_POLICY_FCFS;
        snprintf(plan->reason, sizeof(plan->reason),
                 "experimental: short-job evidence → consider Preemptive FIFO");
        plan->confidence = HAEUN_PLAN_CONF_LOW;
    } else if (plan->workload_class == HAEUN_PLAN_WL_BURSTY &&
               mon->policy_id == HAEUN_EVAL_POLICY_RR && ok &&
               ready_run >= sess->cfg.ready_run_ratio_high) {
        want = HAEUN_EVAL_POLICY_FCFS;
        snprintf(plan->reason, sizeof(plan->reason),
                 "experimental: high ready/run under RR → consider FIFO");
        plan->confidence = HAEUN_PLAN_CONF_LOW;
    } else if ((plan->workload_class == HAEUN_PLAN_WL_CPU_BOUND ||
                plan->workload_class == HAEUN_PLAN_WL_MIXED) &&
               mon->policy_id == HAEUN_EVAL_POLICY_FCFS) {
        want = HAEUN_EVAL_POLICY_RR;
        snprintf(plan->reason, sizeof(plan->reason),
                 "experimental: CPU/mixed under FIFO → consider RR fairness");
        plan->confidence = HAEUN_PLAN_CONF_LOW;
    } else {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->confidence = HAEUN_PLAN_CONF_MEDIUM;
        snprintf(plan->reason, sizeof(plan->reason),
                 "evidence does not justify change from current policy");
        want = mon->policy_id;
    }
    if (want == mon->policy_id) {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->target_policy = mon->policy_id;
        sess->consecutive_same = 0u;
        snprintf(plan->stability_status, sizeof(plan->stability_status), "n/a");
        push_hist(sess, plan->decision, plan->target_policy);
        sess->last_decision_ticks = mon->timer_count;
        sess->last_policy = mon->policy_id;
        return HAEUN_PLAN_OK;
    }
    /* Hysteresis: marginal ratio changes keep current. */
    if (ok && ready_run >= 0.0) {
        double margin = ready_run - sess->cfg.ready_run_ratio_high;
        if (margin >= 0.0 && margin < sess->cfg.hysteresis_delta &&
            want == HAEUN_EVAL_POLICY_FCFS) {
            plan->decision = HAEUN_PLAN_KEEP_CURRENT;
            plan->target_policy = mon->policy_id;
            snprintf(plan->reason, sizeof(plan->reason),
                     "hysteresis: ready/run within delta of threshold");
            snprintf(plan->stability_status, sizeof(plan->stability_status),
                     "hysteresis");
            push_hist(sess, plan->decision, plan->target_policy);
            return HAEUN_PLAN_OK;
        }
    }
    if (sess->last_recommend_target == want)
        sess->consecutive_same++;
    else {
        sess->consecutive_same = 1u;
        sess->last_recommend_target = want;
    }
    if (stability_suppress(sess, mon, want, stab, sizeof(stab))) {
        plan->decision = HAEUN_PLAN_KEEP_CURRENT;
        plan->target_policy = mon->policy_id;
        snprintf(plan->stability_status, sizeof(plan->stability_status), "%s",
                 stab);
        snprintf(plan->reason, sizeof(plan->reason),
                 "change interest suppressed by stability");
        snprintf(plan->stability_status, sizeof(plan->stability_status), "%s",
                 stab);
        push_hist(sess, plan->decision, plan->target_policy);
        return HAEUN_PLAN_OK;
    }
    plan->decision = HAEUN_PLAN_RECOMMEND_CHANGE;
    plan->target_policy = want;
    snprintf(plan->stability_status, sizeof(plan->stability_status), "ok");
    push_hist(sess, plan->decision, plan->target_policy);
    sess->last_decision_ticks = mon->timer_count;
    sess->last_policy = mon->policy_id;
    return HAEUN_PLAN_OK;
}
