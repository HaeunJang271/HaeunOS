#ifndef HAEUN_EVAL_REPORT_H
#define HAEUN_EVAL_REPORT_H

#include "haeun/eval_cost.h"
#include "haeun/eval_fairness.h"
#include "haeun/eval_stats.h"
#include "haeun/eval_types.h"

#include <stdio.h>

/** Aggregates of successful runs for reporting. */
typedef struct haeun_eval_report_bundle {
    const haeun_eval_experiment *exp;
    haeun_eval_stat_summary st_run;
    haeun_eval_stat_summary st_ready;
    haeun_eval_stat_summary st_dispatch;
    haeun_eval_stat_summary st_ctx;
    haeun_eval_fairness fairness_first_ok;
    int fairness_available;
    haeun_eval_cost_result cost;
    int cost_available;
} haeun_eval_report_bundle;

/**
 * Fills aggregate stats from successful runs; optional cost if weights != NULL.
 */
haeun_eval_result haeun_eval_report_prepare(const haeun_eval_experiment *exp,
                                            const haeun_eval_cost_weights *weights,
                                            haeun_eval_report_bundle *out);

/**
 * Prints a human-readable evaluation report.
 */
void haeun_eval_report_fprint(FILE *fp, const haeun_eval_report_bundle *b);

/**
 * Prints schema-versioned JSON suitable for later automation.
 */
void haeun_eval_report_fprint_json(FILE *fp, const haeun_eval_report_bundle *b);

#endif
