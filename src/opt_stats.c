#include "haeun/optimize.h"

#include "haeun/eval_stats.h"

#include <stdio.h>
#include <string.h>

static void opt_na(haeun_eval_opt *o, const char *why) {
    o->available = 0;
    o->value = 0.0;
    snprintf(o->reason, sizeof(o->reason), "%s", why ? why : "n/a");
}

static void opt_set(haeun_eval_opt *o, double v) {
    o->available = 1;
    o->value = v;
    o->reason[0] = '\0';
}

static double isqrt(double x) {
    double g;
    int i;
    if (x <= 0.0)
        return 0.0;
    g = x > 1.0 ? x : 1.0;
    for (i = 0; i < 24; ++i)
        g = 0.5 * (g + x / g);
    return g;
}

/**
 * Fills metric stats from sample array; missing tracked separately.
 */
haeun_opt_result haeun_opt_stats_fill(const double *vals, unsigned n,
                                      unsigned missing, const char *name,
                                      haeun_opt_metric_stats *out) {
    haeun_eval_stat_summary s;
    if (out == NULL)
        return HAEUN_OPT_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (name != NULL)
        snprintf(out->name, sizeof(out->name), "%s", name);
    out->n = n;
    out->missing = missing;
    if (haeun_eval_stats_compute(vals, n, &s) != HAEUN_EVAL_OK)
        return HAEUN_OPT_BAD_ARGUMENT;
    out->mean = s.mean;
    out->median = s.median;
    out->min = s.min;
    out->max = s.max;
    out->stddev = s.stddev;
    /* Approximate normal CI: mean ± 1.96 * s / √n — only if n≥2 and stddev ok.
     * Not a claim of normality; reported as experimental descriptive interval. */
    if (n >= 2u && s.stddev.available && s.mean.available) {
        double half = 1.96 * s.stddev.value / isqrt((double)n);
        opt_set(&out->ci95_lo, s.mean.value - half);
        opt_set(&out->ci95_hi, s.mean.value + half);
    } else {
        opt_na(&out->ci95_lo, "need n>=2 and stddev");
        opt_na(&out->ci95_hi, "need n>=2 and stddev");
    }
    return HAEUN_OPT_OK;
}
