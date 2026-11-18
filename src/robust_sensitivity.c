#include "haeun/robust.h"

#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

static void perturb(haeun_opt_profile *p, double factor) {
    unsigned i;
    double sum = 0.0;
    for (i = 0; i < p->obj_count; ++i) {
        if (i == 0u)
            p->objs[i].weight *= factor;
        sum += p->objs[i].weight;
    }
    if (sum < 1e-12)
        return;
    for (i = 0; i < p->obj_count; ++i)
        p->objs[i].weight /= sum;
}

/**
 * Sensitivity: perturb first objective weight; re-select on cached candidates.
 */
haeun_rb_result haeun_rb_run_sensitivity(haeun_rb_state *st) {
    haeun_opt_profile base;
    unsigned w;
    if (st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    if (haeun_opt_profile_get(st->cfg.profile_id, &base) != HAEUN_OPT_OK)
        return HAEUN_RB_BAD_ARGUMENT;
    for (w = 0; w < st->wl_count; ++w) {
        haeun_rb_wl_report *wr = &st->workloads[w];
        haeun_opt_wl_result trial;
        haeun_opt_profile p;
        haeun_opt_decision d0, d1, d2;
        uint32_t pol0, pol1, pol2;
        unsigned changes = 0u;
        if (wr->opt.cand_count < 2u) {
            wr->sensitivity = HAEUN_RB_SENS_INSUFFICIENT;
            continue;
        }
        trial = wr->opt;
        p = base;
        (void)haeun_opt_select(&p, &trial);
        d0 = trial.decision;
        pol0 = trial.recommended_policy;
        trial = wr->opt;
        p = base;
        perturb(&p, 1.10);
        (void)haeun_opt_select(&p, &trial);
        d1 = trial.decision;
        pol1 = trial.recommended_policy;
        trial = wr->opt;
        p = base;
        perturb(&p, 1.50);
        (void)haeun_opt_select(&p, &trial);
        d2 = trial.decision;
        pol2 = trial.recommended_policy;
        if (d1 != d0 || pol1 != pol0)
            changes++;
        if (d2 != d0 || pol2 != pol0)
            changes++;
        if (changes == 0u)
            wr->sensitivity = HAEUN_RB_STABLE;
        else if (changes == 1u)
            wr->sensitivity = HAEUN_RB_SENSITIVE;
        else
            wr->sensitivity = HAEUN_RB_UNSTABLE;
        {
            char tmp[HAEUN_RB_MSG_LEN];
            snprintf(tmp, sizeof(tmp), "%s; sensitivity=%d", wr->reason,
                     (int)wr->sensitivity);
            snprintf(wr->reason, sizeof(wr->reason), "%s", tmp);
        }
    }
    return HAEUN_RB_OK;
}

/**
 * Workload robustness: catalog images are fixed; parametric variation
 * beyond step_cap is NOT_SUPPORTED.
 */
haeun_rb_result haeun_rb_workload_robustness(haeun_rb_state *st) {
    unsigned w;
    if (st == NULL)
        return HAEUN_RB_BAD_ARGUMENT;
    for (w = 0; w < st->wl_count; ++w) {
        haeun_rb_wl_report *wr = &st->workloads[w];
        {
            char tmp[HAEUN_RB_MSG_LEN];
            snprintf(tmp, sizeof(tmp),
                     "%s; robustness=baseline_only (NOT_SUPPORTED)", wr->reason);
            snprintf(wr->reason, sizeof(wr->reason), "%s", tmp);
        }
    }
    return HAEUN_RB_OK;
}
