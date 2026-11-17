#ifndef HAEUN_EVAL_COST_H
#define HAEUN_EVAL_COST_H

#include "haeun/eval_types.h"

/** Explicit weights; lower combined score is better after normalization. */
typedef struct haeun_eval_cost_weights {
    double w_ready_wait;
    double w_ctx_ratio;
    double w_imbalance;
    double w_incomplete;
} haeun_eval_cost_weights;

typedef struct haeun_eval_cost_result {
    int experimental;
    haeun_eval_opt score;
    haeun_eval_opt s_ready;
    haeun_eval_opt s_ctx;
    haeun_eval_opt s_imbalance;
    haeun_eval_opt s_incomplete;
    haeun_eval_cost_weights weights;
} haeun_eval_cost_result;

/**
 * Returns neutral demonstration weights (not claimed optimal).
 */
void haeun_eval_cost_default_weights(haeun_eval_cost_weights *w);

/**
 * Experimental host-only cost; never wired to the guest scheduler.
 */
haeun_eval_result haeun_eval_cost_compute(const haeun_eval_run_metrics *m,
                                          const haeun_eval_cost_weights *w,
                                          haeun_eval_cost_result *out);

#endif
