#include "haeun/optimize.h"

#include "haeun/eval_fairness.h"
#include "haeun/eval_runner.h"
#include "haeun/eval_workload.h"
#include "haeun/feedback_experiment.h"

#include <stdio.h>
#include <string.h>

static void fill_from_exp(haeun_opt_candidate *c, const haeun_eval_experiment *ex,
                          unsigned min_success) {
    double ready[HAEUN_EVAL_MAX_RUNS];
    double jain[HAEUN_EVAL_MAX_RUNS];
    double disp[HAEUN_EVAL_MAX_RUNS];
    double comp[HAEUN_EVAL_MAX_RUNS];
    unsigned nr = 0u, nj = 0u, nd = 0u, nc = 0u;
    unsigned miss_r = 0u, miss_j = 0u, i;
    c->reps = ex->requested_reps ? ex->requested_reps : ex->success_count;
    c->success = ex->success_count;
    c->fail = ex->fail_count;
    for (i = 0; i < ex->success_count && i < HAEUN_EVAL_MAX_RUNS; ++i) {
        const haeun_eval_run_metrics *m = &ex->runs[i].metrics;
        haeun_eval_fairness f;
        if (!m->ok) {
            miss_r++;
            miss_j++;
            continue;
        }
        if (m->avg_ready_wait.available)
            ready[nr++] = m->avg_ready_wait.value;
        else
            miss_r++;
        if (haeun_eval_fairness_from_metrics(m, &f) == HAEUN_EVAL_OK &&
            f.jain_run_ticks.available)
            jain[nj++] = f.jain_run_ticks.value;
        else
            miss_j++;
        disp[nd++] = (double)m->total_dispatch;
        comp[nc++] = (double)m->completed;
    }
    (void)haeun_opt_stats_fill(ready, nr, miss_r, "avg_ready_wait", &c->ready);
    (void)haeun_opt_stats_fill(jain, nj, miss_j, "jain_fairness", &c->jain);
    (void)haeun_opt_stats_fill(disp, nd, 0u, "dispatch_count", &c->dispatch);
    (void)haeun_opt_stats_fill(comp, nc, 0u, "completed", &c->completed);
    if (c->fail > 0u && c->success == 0u) {
        c->status = HAEUN_OPT_CAND_INVALID;
        snprintf(c->reason, sizeof(c->reason), "all runs failed");
    } else if (c->success < min_success) {
        c->status = HAEUN_OPT_CAND_INSUFFICIENT;
        snprintf(c->reason, sizeof(c->reason), "success %u < min %u",
                 c->success, min_success);
    } else if (!c->ready.mean.available && !c->jain.mean.available) {
        c->status = HAEUN_OPT_CAND_INCONCLUSIVE;
        snprintf(c->reason, sizeof(c->reason), "core metrics unavailable");
    } else {
        c->status = HAEUN_OPT_CAND_VALID;
        snprintf(c->reason, sizeof(c->reason), "ok");
    }
}

static haeun_opt_result run_fixed(const haeun_opt_config *cfg,
                                  haeun_eval_wl_kind kind, uint32_t pol,
                                  haeun_opt_candidate *c) {
    haeun_eval_workload wl;
    haeun_eval_experiment ex;
    char id[64];
    memset(c, 0, sizeof(*c));
    c->policy_id = pol;
    snprintf(c->name, sizeof(c->name), "%s", pol == 0u ? "RR" : "FIFO");
    if (haeun_eval_workload_get(kind, &wl) != HAEUN_EVAL_OK)
        return HAEUN_OPT_BAD_ARGUMENT;
    snprintf(id, sizeof(id), "r36-%u-%u", (unsigned)kind, pol);
    if (haeun_eval_experiment_run(&wl, cfg->build_dir, pol, id, cfg->reps,
                                  &ex) != HAEUN_EVAL_OK)
        return HAEUN_OPT_IO;
    fill_from_exp(c, &ex, cfg->min_success);
    return HAEUN_OPT_OK;
}

/**
 * Runs RR+FIFO experiments for one workload into wl_result.
 */
haeun_opt_result haeun_opt_run_workload(const haeun_opt_config *cfg,
                                        const haeun_opt_profile *prof,
                                        haeun_eval_wl_kind kind,
                                        haeun_opt_wl_result *out) {
    haeun_eval_workload wl;
    if (cfg == NULL || prof == NULL || out == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    if (haeun_eval_workload_get(kind, &wl) != HAEUN_EVAL_OK)
        return HAEUN_OPT_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->kind = kind;
    snprintf(out->workload_id, sizeof(out->workload_id), "%s", wl.name);
    out->fingerprint = (uint32_t)kind + 1u;
    out->cand_count = 2u;
    if (run_fixed(cfg, kind, 0u, &out->cands[0]) != HAEUN_OPT_OK)
        return HAEUN_OPT_IO;
    if (run_fixed(cfg, kind, 1u, &out->cands[1]) != HAEUN_OPT_OK)
        return HAEUN_OPT_IO;
    return haeun_opt_select(prof, out);
}

/**
 * Runs optimization over all catalog workloads (no APPLY).
 */
haeun_opt_result haeun_opt_run(haeun_opt_state *st) {
    unsigned k;
    if (st == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    if (haeun_opt_profile_get(st->cfg.profile_id, &st->profile) != HAEUN_OPT_OK)
        return HAEUN_OPT_BAD_ARGUMENT;
    st->cycle_id++;
    st->wl_count = 0u;
    memset(&st->last_rec, 0, sizeof(st->last_rec));
    for (k = 0; k < HAEUN_WL_COUNT; ++k) {
        haeun_opt_wl_result *wr = &st->workloads[st->wl_count];
        if (haeun_opt_run_workload(&st->cfg, &st->profile, (haeun_eval_wl_kind)k,
                                   wr) != HAEUN_OPT_OK &&
            wr->decision == HAEUN_OPT_DEC_INCONCLUSIVE && wr->cand_count == 0u)
            continue;
        st->wl_count++;
        if (wr->decision == HAEUN_OPT_DEC_SELECT) {
            st->last_rec.valid = 1;
            st->last_rec.schema_version = HAEUN_OPT_SCHEMA;
            snprintf(st->last_rec.experiment_id,
                     sizeof(st->last_rec.experiment_id), "r36-%u-%s",
                     st->cycle_id, wr->workload_id);
            st->last_rec.profile_id = st->cfg.profile_id;
            st->last_rec.workload = wr->kind;
            st->last_rec.recommended_policy = wr->recommended_policy;
            st->last_rec.decision = wr->decision;
            st->last_rec.generation = st->cycle_id;
            st->last_rec.config_fingerprint = st->cfg.config_fingerprint;
            snprintf(st->last_rec.reason, sizeof(st->last_rec.reason), "%s",
                     wr->reason);
            snprintf(st->last_rec.limitations, sizeof(st->last_rec.limitations),
                     "last SELECT wins; per-workload rows are authoritative");
        }
    }
    if (st->hist_count < HAEUN_OPT_HIST_MAX && st->last_rec.valid)
        st->history[st->hist_count++] = st->last_rec;
    return HAEUN_OPT_OK;
}
