#include "haeun/planner.h"

#include <stdio.h>
#include <string.h>

static void add_ev(haeun_plan *plan, const char *name, haeun_plan_ev_kind kind,
                   double v, const char *note) {
    haeun_plan_evidence *e;
    if (plan->evidence_count >= HAEUN_PLAN_EVIDENCE_MAX)
        return;
    e = &plan->evidence[plan->evidence_count++];
    snprintf(e->name, sizeof(e->name), "%s", name);
    e->kind = kind;
    e->value = v;
    snprintf(e->note, sizeof(e->note), "%s", note);
}

static void add_opt(haeun_plan *plan, const char *name, const haeun_eval_opt *o) {
    if (!o->available)
        add_ev(plan, name, HAEUN_PLAN_EV_MISSING, 0.0, o->reason);
    else if (o->value == 0.0)
        add_ev(plan, name, HAEUN_PLAN_EV_ZERO, 0.0, "measured zero");
    else
        add_ev(plan, name, HAEUN_PLAN_EV_OK, o->value, "available");
}

/**
 * Classifies workload from monitor evidence into wl_class + evidence[].
 */
haeun_plan_result haeun_plan_analyze(const haeun_plan_obs *mon,
                                     const haeun_plan_config *cfg,
                                     haeun_plan *plan) {
    uint64_t run, ready, sleep, blocked, disp, sum;
    double sleep_share, ready_run, disp_run;
    unsigned ready_n = 0u, run_n = 0u, sleep_n = 0u, wait_n = 0u, i;
    if (mon == NULL || cfg == NULL || plan == NULL)
        return HAEUN_PLAN_BAD_ARGUMENT;
    plan->evidence_count = 0u;
    plan->workload_class = HAEUN_PLAN_WL_UNKNOWN;
    if (!mon->telem_ok) {
        add_ev(plan, "telem", HAEUN_PLAN_EV_MISSING, 0.0, "telemetry unavailable");
        return HAEUN_PLAN_NO_TELEM;
    }
    if (!mon->policy_ok) {
        add_ev(plan, "policy", HAEUN_PLAN_EV_INVALID, (double)mon->policy_id,
               "unrecognized policy ID");
        return HAEUN_PLAN_OK;
    }
    add_ev(plan, "policy_id", HAEUN_PLAN_EV_OK, (double)mon->policy_id, "active");
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        const haeun_telem_slot *s = &mon->telem.slots[i];
        if (!s->as_slot_valid)
            continue;
        if (s->state == 1u)
            ready_n++;
        else if (s->state == 2u)
            run_n++;
        else if (s->state == 3u)
            sleep_n++;
        else if (s->state == 4u)
            wait_n++;
    }
    add_ev(plan, "n_ready", ready_n ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO,
           (double)ready_n, "READY slots");
    add_ev(plan, "n_running", run_n ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO,
           (double)run_n, "RUNNING slots");
    if (!mon->metrics_ok) {
        add_ev(plan, "metrics", HAEUN_PLAN_EV_UNKNOWN, 0.0,
               "eval metrics not available");
        plan->workload_class = HAEUN_PLAN_WL_UNKNOWN;
        return HAEUN_PLAN_OK;
    }
    run = mon->metrics.total_run_ticks;
    ready = mon->metrics.total_ready_wait;
    sleep = mon->metrics.total_sleep;
    blocked = mon->metrics.total_blocked;
    disp = mon->metrics.total_dispatch;
    sum = run + ready + sleep + blocked;
    add_ev(plan, "total_run", run ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO,
           (double)run, "guest ticks");
    add_ev(plan, "total_ready_wait", ready ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO,
           (double)ready, "guest ticks");
    add_ev(plan, "total_sleep", sleep ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO,
           (double)sleep, "guest ticks");
    add_ev(plan, "total_blocked",
           blocked ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO, (double)blocked,
           "guest ticks");
    add_ev(plan, "total_dispatch",
           disp ? HAEUN_PLAN_EV_OK : HAEUN_PLAN_EV_ZERO, (double)disp, "count");
    add_opt(plan, "avg_ready_wait", &mon->metrics.avg_ready_wait);
    if (sum == 0u && disp == 0u && ready_n == 0u && run_n == 0u) {
        plan->workload_class = HAEUN_PLAN_WL_IDLE;
        return HAEUN_PLAN_OK;
    }
    sleep_share = (sum > 0u) ? ((double)(sleep + blocked) / (double)sum) : 0.0;
    ready_run = (run > 0u) ? ((double)ready / (double)run) : -1.0;
    disp_run = (run > 0u) ? ((double)disp / (double)run) : -1.0;
    if (ready_run >= 0.0)
        add_ev(plan, "ready_run_ratio", HAEUN_PLAN_EV_OK, ready_run,
               "ready_wait/run");
    else
        add_ev(plan, "ready_run_ratio", HAEUN_PLAN_EV_NA, 0.0, "run==0");
    if (disp_run >= 0.0)
        add_ev(plan, "dispatch_run_ratio", HAEUN_PLAN_EV_OK, disp_run,
               "dispatch/run");
    else
        add_ev(plan, "dispatch_run_ratio", HAEUN_PLAN_EV_NA, 0.0, "run==0");
    add_ev(plan, "sleep_block_share", HAEUN_PLAN_EV_OK, sleep_share,
           "(sleep+blocked)/sum");
    if (wait_n > 0u && sleep_share >= cfg->sleep_share_high)
        plan->workload_class = HAEUN_PLAN_WL_BLOCK_WAIT;
    else if (sleep_n > 0u && sleep_share >= cfg->sleep_share_high)
        plan->workload_class = HAEUN_PLAN_WL_BLOCK_SLEEP;
    else if (disp_run >= 0.0 && disp_run >= cfg->dispatch_run_high &&
             run > 0u && run < 5000u)
        plan->workload_class = HAEUN_PLAN_WL_SHORT_JOB;
    else if (ready_run >= 0.0 && ready_run >= cfg->ready_run_ratio_high)
        plan->workload_class = HAEUN_PLAN_WL_BURSTY;
    else if (run > 0u && sleep_share < 0.15 &&
             (ready_run < 0.0 || ready_run < 1.0))
        plan->workload_class = HAEUN_PLAN_WL_CPU_BOUND;
    else if (run > 0u && (sleep > 0u || ready > 0u))
        plan->workload_class = HAEUN_PLAN_WL_MIXED;
    else
        plan->workload_class = HAEUN_PLAN_WL_UNKNOWN;
    return HAEUN_PLAN_OK;
}
