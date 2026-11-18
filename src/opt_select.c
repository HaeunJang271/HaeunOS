#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

/**
 * Selects recommendation for one workload result under profile.
 */
haeun_opt_result haeun_opt_select(const haeun_opt_profile *prof,
                                  haeun_opt_wl_result *wl) {
    unsigned i;
    int vi = -1, vj = -1;
    double gap;
    double improve;
    if (prof == NULL || wl == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    wl->decision = HAEUN_OPT_DEC_INCONCLUSIVE;
    wl->recommended_policy = 0u;
    haeun_opt_pareto_mark(wl->cands, wl->cand_count);
    for (i = 0; i < wl->cand_count; ++i) {
        if (wl->cands[i].status == HAEUN_OPT_CAND_VALID) {
            if (vi < 0)
                vi = (int)i;
            else if (vj < 0)
                vj = (int)i;
        }
    }
    if (vi < 0) {
        wl->decision = HAEUN_OPT_DEC_INCONCLUSIVE;
        snprintf(wl->reason, sizeof(wl->reason), "no VALID candidates");
        return HAEUN_OPT_INCONCLUSIVE;
    }
    if (vj < 0) {
        wl->decision = HAEUN_OPT_DEC_NO_DECISION;
        snprintf(wl->reason, sizeof(wl->reason),
                 "only one VALID candidate; need comparison");
        return HAEUN_OPT_OK;
    }
    haeun_opt_score_pair(prof, &wl->cands[vi], &wl->cands[vj]);
    if (!wl->cands[vi].score_valid || !wl->cands[vj].score_valid) {
        wl->decision = HAEUN_OPT_DEC_INCONCLUSIVE;
        snprintf(wl->reason, sizeof(wl->reason),
                 "insufficient comparable objectives");
        return HAEUN_OPT_INCONCLUSIVE;
    }
    gap = wl->cands[vi].score - wl->cands[vj].score;
    improve = prof->min_improve;
    if (gap > improve) {
        wl->decision = HAEUN_OPT_DEC_SELECT;
        wl->recommended_policy = wl->cands[vi].policy_id;
        snprintf(wl->reason, sizeof(wl->reason),
                 "select %s score=%.4f vs %.4f (gap>%.3f)", wl->cands[vi].name,
                 wl->cands[vi].score, wl->cands[vj].score, improve);
        return HAEUN_OPT_OK;
    }
    if (gap < -improve) {
        wl->decision = HAEUN_OPT_DEC_SELECT;
        wl->recommended_policy = wl->cands[vj].policy_id;
        snprintf(wl->reason, sizeof(wl->reason),
                 "select %s score=%.4f vs %.4f (gap>%.3f)", wl->cands[vj].name,
                 wl->cands[vj].score, wl->cands[vi].score, improve);
        return HAEUN_OPT_OK;
    }
    wl->decision = HAEUN_OPT_DEC_EQUIVALENT;
    snprintf(wl->reason, sizeof(wl->reason),
             "scores within min_improve (%.4f vs %.4f)", wl->cands[vi].score,
             wl->cands[vj].score);
    return HAEUN_OPT_OK;
}
