#ifndef HAEUN_EVAL_FAIRNESS_H
#define HAEUN_EVAL_FAIRNESS_H

#include "haeun/eval_types.h"

/** Fairness view over participating run_ticks (CPU allocation). */
typedef struct haeun_eval_fairness {
    unsigned n;
    uint32_t min_run;
    uint32_t max_run;
    haeun_eval_opt jain_run_ticks;
    char notes[HAEUN_EVAL_REASON_MAX];
} haeun_eval_fairness;

/**
 * Computes CPU-allocation fairness from included slot run_ticks.
 * Jain index includes zeros; documents interpretation in notes.
 */
haeun_eval_result haeun_eval_fairness_from_metrics(const haeun_eval_run_metrics *m,
                                                   haeun_eval_fairness *out);

#endif
