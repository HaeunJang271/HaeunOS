#include "haeun/robust.h"

#include "haeun/eval_runner.h"
#include "haeun/eval_workload.h"
#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

static void fp_from_metrics(haeun_rb_run_fp *fp,
                            const haeun_eval_run_metrics *m) {
    memset(fp, 0, sizeof(*fp));
    if (m == NULL || !m->ok) {
        fp->ok = 0;
        if (m)
            snprintf(fp->fail_reason, sizeof(fp->fail_reason), "%s",
                     m->fail_reason);
        return;
    }
    fp->ok = 1;
    fp->exit_code = m->exit_code;
    fp->total_dispatch = m->total_dispatch;
    fp->context_switches = m->context_switches;
    fp->timer_irqs = m->timer_irqs;
    fp->total_run = m->total_run_ticks;
    fp->total_ready = m->total_ready_wait;
    fp->completed = m->completed;
}

static int push_exp(haeun_rb_state *st, const haeun_rb_experiment *e) {
    if (st->exp_count >= HAEUN_RB_REC_MAX)
        return 0;
    st->experiments[st->exp_count++] = *e;
    return 1;
}

static haeun_rb_result record_reps(haeun_rb_state *st, haeun_eval_wl_kind kind,
                                   uint32_t policy, haeun_rb_run_fp *first,
                                   haeun_rb_run_fp *second) {
    haeun_eval_workload wl;
    unsigned r;
    if (haeun_eval_workload_get(kind, &wl) != HAEUN_EVAL_OK)
        return HAEUN_RB_BAD_ARGUMENT;
    memset(first, 0, sizeof(*first));
    memset(second, 0, sizeof(*second));
    for (r = 0; r < st->cfg.reps; ++r) {
        haeun_eval_run_metrics m;
        haeun_rb_experiment ex;
        haeun_rb_run_fp fp;
        memset(&ex, 0, sizeof(ex));
        if (haeun_eval_run_once(&wl, st->cfg.build_dir, policy, &m) !=
            HAEUN_EVAL_OK) {
            fp_from_metrics(&fp, &m);
        } else {
            fp_from_metrics(&fp, &m);
        }
        snprintf(ex.experiment_id, sizeof(ex.experiment_id), "r37-%u-%u-%u",
                 (unsigned)kind, policy, r);
        ex.config_hash = st->cfg.config_hash;
        ex.workload_seed = haeun_rb_workload_seed(kind, policy);
        ex.workload = kind;
        ex.policy_id = policy;
        ex.repetition = r;
        ex.ok = fp.ok;
        ex.fp = fp;
        (void)push_exp(st, &ex);
        if (r == 0u)
            *first = fp;
        else if (r == 1u)
            *second = fp;
    }
    return HAEUN_RB_OK;
}

/**
 * Runs repeated RR/FIFO experiments + replay/reliability for all workloads.
 */
haeun_rb_result haeun_rb_run_experiments(haeun_rb_state *st) {
    haeun_opt_config ocfg;
    haeun_opt_profile prof;
    unsigned k;
    if (st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    st->cfg.config_hash = haeun_rb_config_hash(&st->cfg);
    st->wl_count = 0u;
    st->exp_count = 0u;
    haeun_opt_config_default(&ocfg);
    snprintf(ocfg.build_dir, sizeof(ocfg.build_dir), "%s", st->cfg.build_dir);
    ocfg.reps = st->cfg.reps;
    ocfg.min_success = st->cfg.min_success;
    ocfg.profile_id = st->cfg.profile_id;
    if (haeun_opt_profile_get(st->cfg.profile_id, &prof) != HAEUN_OPT_OK)
        return HAEUN_RB_BAD_ARGUMENT;
    for (k = 0; k < HAEUN_WL_COUNT; ++k) {
        haeun_rb_wl_report *wr = &st->workloads[st->wl_count];
        haeun_rb_run_fp a0, a1, b0, b1;
        haeun_eval_workload wl;
        memset(wr, 0, sizeof(*wr));
        wr->kind = (haeun_eval_wl_kind)k;
        if (haeun_eval_workload_get(wr->kind, &wl) != HAEUN_EVAL_OK)
            continue;
        snprintf(wr->workload_id, sizeof(wr->workload_id), "%s", wl.name);
        wr->seed = haeun_rb_workload_seed(wr->kind, 0u);
        if (haeun_opt_run_workload(&ocfg, &prof, wr->kind, &wr->opt) !=
            HAEUN_OPT_OK) {
            wr->reliability = HAEUN_RB_REL_INCONCLUSIVE;
            snprintf(wr->reason, sizeof(wr->reason), "opt run inconclusive");
        }
        wr->original_decision = wr->opt.decision;
        wr->original_policy = wr->opt.recommended_policy;
        (void)record_reps(st, wr->kind, 0u, &a0, &a1);
        (void)record_reps(st, wr->kind, 1u, &b0, &b1);
        wr->replay = haeun_rb_compare_fp(&a0, &a1);
        if (wr->replay == HAEUN_RB_EXACT_MATCH ||
            wr->replay == HAEUN_RB_METRIC_MATCH) {
            haeun_rb_replay_class br = haeun_rb_compare_fp(&b0, &b1);
            if (br == HAEUN_RB_DIVERGENT || br == HAEUN_RB_INVALID)
                wr->replay = br;
        }
        wr->reliability = HAEUN_RB_REL_SUFFICIENT;
        if (wr->opt.cand_count >= 1u) {
            haeun_rb_reliability ra =
                haeun_rb_classify_reliability(&wr->opt.cands[0],
                                              st->cfg.min_success);
            haeun_rb_reliability rb =
                wr->opt.cand_count >= 2u
                    ? haeun_rb_classify_reliability(&wr->opt.cands[1],
                                                    st->cfg.min_success)
                    : HAEUN_RB_REL_INSUFFICIENT;
            if (ra == HAEUN_RB_REL_INVALID || rb == HAEUN_RB_REL_INVALID)
                wr->reliability = HAEUN_RB_REL_INVALID;
            else if (ra != HAEUN_RB_REL_SUFFICIENT ||
                     rb != HAEUN_RB_REL_SUFFICIENT)
                wr->reliability = HAEUN_RB_REL_INSUFFICIENT;
        }
        snprintf(wr->reason, sizeof(wr->reason), "replay=%d rel=%d dec=%d",
                 (int)wr->replay, (int)wr->reliability, (int)wr->opt.decision);
        st->wl_count++;
    }
    return HAEUN_RB_OK;
}
