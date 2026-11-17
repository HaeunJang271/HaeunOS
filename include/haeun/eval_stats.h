#ifndef HAEUN_EVAL_STATS_H
#define HAEUN_EVAL_STATS_H

#include "haeun/eval_types.h"

/** Aggregate summary over a sample of doubles. */
typedef struct haeun_eval_stat_summary {
    unsigned sample_count;
    haeun_eval_opt mean;
    haeun_eval_opt median;
    haeun_eval_opt min;
    haeun_eval_opt max;
    haeun_eval_opt stddev;
} haeun_eval_stat_summary;

/**
 * Computes mean/median/min/max/sample stddev for values[0..n).
 * Empty input yields unavailable metrics with reasons.
 */
haeun_eval_result haeun_eval_stats_compute(const double *values, unsigned n,
                                           haeun_eval_stat_summary *out);

#endif
