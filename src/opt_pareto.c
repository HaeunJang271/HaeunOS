#include "haeun/optimize.h"

#include <string.h>

static int better(double a, double b, haeun_opt_dir dir) {
    if (dir == HAEUN_OPT_DIR_MIN)
        return a < b - 1e-12;
    return a > b + 1e-12;
}

static int worse_or_eq(double a, double b, haeun_opt_dir dir) {
    if (dir == HAEUN_OPT_DIR_MIN)
        return a >= b - 1e-12;
    return a <= b + 1e-12;
}

static const haeun_opt_metric_stats *metric(const haeun_opt_candidate *c,
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

/** Returns 1 if A dominates B on available shared metrics (strict on ≥1). */
static int dominates(const haeun_opt_candidate *a, const haeun_opt_candidate *b,
                     const char *const *names, const haeun_opt_dir *dirs,
                     unsigned nobj) {
    unsigned i;
    int strict = 0;
    unsigned compared = 0u;
    for (i = 0; i < nobj; ++i) {
        const haeun_opt_metric_stats *ma = metric(a, names[i]);
        const haeun_opt_metric_stats *mb = metric(b, names[i]);
        if (ma == NULL || mb == NULL || !ma->mean.available || !mb->mean.available)
            continue;
        compared++;
        if (worse_or_eq(ma->mean.value, mb->mean.value, dirs[i]) &&
            !better(ma->mean.value, mb->mean.value, dirs[i])) {
            /* a not better on this metric */
            if (better(mb->mean.value, ma->mean.value, dirs[i]))
                return 0; /* b better on this → a cannot dominate */
        } else if (better(ma->mean.value, mb->mean.value, dirs[i])) {
            strict = 1;
        }
    }
    return compared >= 1u && strict;
}

/**
 * Marks Pareto non-dominated among candidates with VALID status.
 * Uses fixed objective axes: ready(min), jain(max), dispatch(min).
 */
void haeun_opt_pareto_mark(haeun_opt_candidate *cands, unsigned n) {
    static const char *names[] = {"avg_ready_wait", "jain_fairness",
                                  "dispatch_count"};
    static const haeun_opt_dir dirs[] = {HAEUN_OPT_DIR_MIN, HAEUN_OPT_DIR_MAX,
                                        HAEUN_OPT_DIR_MIN};
    unsigned i, j;
    if (cands == NULL)
        return;
    for (i = 0; i < n; ++i)
        cands[i].pareto_nondominated = 0;
    for (i = 0; i < n; ++i) {
        int dominated = 0;
        if (cands[i].status != HAEUN_OPT_CAND_VALID)
            continue;
        for (j = 0; j < n; ++j) {
            if (i == j || cands[j].status != HAEUN_OPT_CAND_VALID)
                continue;
            if (dominates(&cands[j], &cands[i], names, dirs, 3u)) {
                dominated = 1;
                break;
            }
        }
        cands[i].pareto_nondominated = dominated ? 0 : 1;
    }
}
