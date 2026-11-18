#include "haeun/robust.h"

#include <string.h>

/**
 * Compares two run fingerprints (metric-level; ignores wall-clock).
 *
 * EXACT_MATCH: all compared counters equal and both ok.
 * METRIC_MATCH: totals match but exit_code differs (should be rare).
 * DIVERGENT: ok but metrics differ.
 * INVALID: either not ok.
 * INCOMPLETE: null arguments.
 */
haeun_rb_replay_class haeun_rb_compare_fp(const haeun_rb_run_fp *a,
                                          const haeun_rb_run_fp *b) {
    if (a == NULL || b == NULL)
        return HAEUN_RB_INCOMPLETE;
    if (!a->ok || !b->ok)
        return HAEUN_RB_INVALID;
    if (a->total_dispatch == b->total_dispatch &&
        a->context_switches == b->context_switches &&
        a->timer_irqs == b->timer_irqs && a->total_run == b->total_run &&
        a->total_ready == b->total_ready && a->completed == b->completed &&
        a->exit_code == b->exit_code)
        return HAEUN_RB_EXACT_MATCH;
    if (a->total_dispatch == b->total_dispatch &&
        a->total_run == b->total_run && a->total_ready == b->total_ready &&
        a->completed == b->completed)
        return HAEUN_RB_METRIC_MATCH;
    return HAEUN_RB_DIVERGENT;
}

/**
 * Classifies statistical reliability from candidate sample counts.
 */
haeun_rb_reliability haeun_rb_classify_reliability(const haeun_opt_candidate *c,
                                                   unsigned min_n) {
    unsigned n;
    if (c == NULL)
        return HAEUN_RB_REL_INVALID;
    if (c->status == HAEUN_OPT_CAND_INVALID)
        return HAEUN_RB_REL_INVALID;
    if (c->status == HAEUN_OPT_CAND_INSUFFICIENT ||
        c->status == HAEUN_OPT_CAND_INCONCLUSIVE)
        return HAEUN_RB_REL_INSUFFICIENT;
    n = c->ready.n;
    if (c->jain.n < n)
        n = c->jain.n;
    if (n < min_n)
        return HAEUN_RB_REL_INSUFFICIENT;
    if (!c->ready.mean.available && !c->jain.mean.available)
        return HAEUN_RB_REL_INCONCLUSIVE;
    /* Descriptive sufficiency only — not inferential significance. */
    return HAEUN_RB_REL_SUFFICIENT;
}
