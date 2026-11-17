#include "haeun/eval_fairness.h"

#include <stdio.h>
#include <string.h>

/**
 * Computes CPU-allocation fairness from included slot run_ticks.
 */
haeun_eval_result haeun_eval_fairness_from_metrics(const haeun_eval_run_metrics *m,
                                                   haeun_eval_fairness *out) {
    unsigned i;
    unsigned n = 0u;
    double sum = 0.0;
    double sum_sq = 0.0;
    uint32_t mn = 0u;
    uint32_t mx = 0u;
    int have = 0;
    if (m == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    snprintf(out->notes, sizeof(out->notes),
             "Jain on run_ticks of included slots; zeros included; "
             "CPU allocation not completion fairness");
    if (!m->ok) {
        out->jain_run_ticks.available = 0;
        snprintf(out->jain_run_ticks.reason, sizeof(out->jain_run_ticks.reason),
                 "run metrics not ok");
        return HAEUN_EVAL_OK;
    }
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        double x;
        if (!m->slot_included[i])
            continue;
        x = (double)m->slot_run[i];
        if (!have || m->slot_run[i] < mn)
            mn = m->slot_run[i];
        if (!have || m->slot_run[i] > mx)
            mx = m->slot_run[i];
        have = 1;
        sum += x;
        sum_sq += x * x;
        n++;
    }
    out->n = n;
    out->min_run = mn;
    out->max_run = mx;
    if (n == 0u) {
        out->jain_run_ticks.available = 0;
        snprintf(out->jain_run_ticks.reason, sizeof(out->jain_run_ticks.reason),
                 "no participants");
        return HAEUN_EVAL_OK;
    }
    if (sum_sq == 0.0) {
        /* All zeros: perfectly equal zero allocation. */
        out->jain_run_ticks.available = 1;
        out->jain_run_ticks.value = 1.0;
        return HAEUN_EVAL_OK;
    }
    out->jain_run_ticks.available = 1;
    out->jain_run_ticks.value = (sum * sum) / ((double)n * sum_sq);
    return HAEUN_EVAL_OK;
}
