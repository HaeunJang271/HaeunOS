#include "haeun/eval_stats.h"

#include <stdio.h>
#include <string.h>

static void opt_set(haeun_eval_opt *o, double v) {
    o->available = 1;
    o->value = v;
    o->reason[0] = '\0';
}

static void opt_na(haeun_eval_opt *o, const char *why) {
    o->available = 0;
    o->value = 0.0;
    snprintf(o->reason, sizeof(o->reason), "%s", why);
}

static double eval_sqrt(double x) {
    double g;
    int i;
    if (x <= 0.0)
        return 0.0;
    g = x > 1.0 ? x : 1.0;
    for (i = 0; i < 24; ++i)
        g = 0.5 * (g + x / g);
    return g;
}

static void sort_copy(const double *in, unsigned n, double *out) {
    unsigned i, j;
    for (i = 0; i < n; ++i)
        out[i] = in[i];
    for (i = 1; i < n; ++i) {
        double key = out[i];
        j = i;
        while (j > 0u && out[j - 1u] > key) {
            out[j] = out[j - 1u];
            j--;
        }
        out[j] = key;
    }
}

/**
 * Computes mean/median/min/max/sample stddev for values[0..n).
 */
haeun_eval_result haeun_eval_stats_compute(const double *values, unsigned n,
                                           haeun_eval_stat_summary *out) {
    double sorted[HAEUN_EVAL_MAX_RUNS];
    double sum = 0.0;
    double var = 0.0;
    unsigned i;
    if (out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->sample_count = n;
    if (values == NULL || n == 0u) {
        opt_na(&out->mean, "empty sample");
        opt_na(&out->median, "empty sample");
        opt_na(&out->min, "empty sample");
        opt_na(&out->max, "empty sample");
        opt_na(&out->stddev, "empty sample");
        return HAEUN_EVAL_OK;
    }
    if (n > HAEUN_EVAL_MAX_RUNS)
        return HAEUN_EVAL_BAD_ARGUMENT;
    for (i = 0; i < n; ++i)
        sum += values[i];
    opt_set(&out->mean, sum / (double)n);
    sort_copy(values, n, sorted);
    opt_set(&out->min, sorted[0]);
    opt_set(&out->max, sorted[n - 1u]);
    if ((n & 1u) != 0u)
        opt_set(&out->median, sorted[n / 2u]);
    else
        opt_set(&out->median, 0.5 * (sorted[n / 2u - 1u] + sorted[n / 2u]));
    if (n < 2u) {
        opt_na(&out->stddev, "need at least 2 samples for sample stddev");
        return HAEUN_EVAL_OK;
    }
    for (i = 0; i < n; ++i) {
        double d = values[i] - out->mean.value;
        var += d * d;
    }
    opt_set(&out->stddev, eval_sqrt(var / (double)(n - 1u)));
    return HAEUN_EVAL_OK;
}
