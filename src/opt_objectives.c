#include "haeun/optimize.h"

#include <stdio.h>
#include <string.h>

/**
 * Normalizes metric pair into [0,1] where 1 is better under direction.
 */
int haeun_opt_normalize_pair(double a, double b, haeun_opt_dir dir, double *na,
                             double *nb) {
    double lo, hi, span;
    if (na == NULL || nb == NULL)
        return 0;
    lo = a < b ? a : b;
    hi = a > b ? a : b;
    span = hi - lo;
    if (span < 1e-12) {
        *na = 0.5;
        *nb = 0.5;
        return 1;
    }
    if (dir == HAEUN_OPT_DIR_MIN) {
        *na = (hi - a) / span;
        *nb = (hi - b) / span;
    } else {
        *na = (a - lo) / span;
        *nb = (b - lo) / span;
    }
    return 1;
}

static const haeun_opt_metric_stats *pick(const haeun_opt_candidate *c,
                                         const char *name) {
    if (strcmp(name, "avg_ready_wait") == 0)
        return &c->ready;
    if (strcmp(name, "jain_fairness") == 0)
        return &c->jain;
    if (strcmp(name, "dispatch_count") == 0)
        return &c->dispatch;
    if (strcmp(name, "completed") == 0)
        return &c->completed;
    return NULL;
}

/**
 * Scores a candidate under profile using available means.
 * Single-candidate: score = sum(weight) for available objs only (relative later).
 */
haeun_opt_result haeun_opt_score_candidate(const haeun_opt_profile *prof,
                                           haeun_opt_candidate *cand) {
    unsigned i;
    double wsum = 0.0;
    unsigned used = 0u;
    if (prof == NULL || cand == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    cand->score = 0.0;
    cand->score_valid = 0;
    if (cand->status != HAEUN_OPT_CAND_VALID)
        return HAEUN_OPT_OK;
    for (i = 0; i < prof->obj_count; ++i) {
        const haeun_opt_metric_stats *m = pick(cand, prof->objs[i].metric_name);
        if (m == NULL || !m->mean.available || m->n < prof->objs[i].min_samples)
            continue;
        /* Placeholder absolute score; pairwise renormalized in select. */
        cand->score += prof->objs[i].weight * m->mean.value;
        wsum += prof->objs[i].weight;
        used++;
    }
    if (used < 1u || wsum < 1e-12) {
        snprintf(cand->reason, sizeof(cand->reason), "insufficient objectives");
        cand->status = HAEUN_OPT_CAND_INSUFFICIENT;
        return HAEUN_OPT_OK;
    }
    cand->score_valid = 1;
    return HAEUN_OPT_OK;
}

/**
 * Pairwise relative scores for exactly two valid candidates under profile.
 */
void haeun_opt_score_pair(const haeun_opt_profile *prof,
                          haeun_opt_candidate *a, haeun_opt_candidate *b) {
    unsigned i;
    double sa = 0.0, sb = 0.0, wsum = 0.0;
    unsigned used = 0u;
    if (prof == NULL || a == NULL || b == NULL)
        return;
    a->score_valid = 0;
    b->score_valid = 0;
    for (i = 0; i < prof->obj_count; ++i) {
        const haeun_opt_metric_stats *ma = pick(a, prof->objs[i].metric_name);
        const haeun_opt_metric_stats *mb = pick(b, prof->objs[i].metric_name);
        double na, nb;
        if (ma == NULL || mb == NULL || !ma->mean.available ||
            !mb->mean.available || ma->n < prof->objs[i].min_samples ||
            mb->n < prof->objs[i].min_samples)
            continue;
        if (!haeun_opt_normalize_pair(ma->mean.value, mb->mean.value,
                                      prof->objs[i].direction, &na, &nb))
            continue;
        sa += prof->objs[i].weight * na;
        sb += prof->objs[i].weight * nb;
        wsum += prof->objs[i].weight;
        used++;
    }
    if (used < 1u || wsum < 1e-12)
        return;
    a->score = sa / wsum;
    b->score = sb / wsum;
    a->score_valid = 1;
    b->score_valid = 1;
}
