#include "haeun/eval_cost.h"

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

/**
 * Returns neutral demonstration weights (not claimed optimal).
 */
void haeun_eval_cost_default_weights(haeun_eval_cost_weights *w) {
    if (w == NULL)
        return;
    w->w_ready_wait = 1.0;
    w->w_ctx_ratio = 1.0;
    w->w_imbalance = 1.0;
    w->w_incomplete = 1.0;
}

/**
 * Experimental host-only cost; never wired to the guest scheduler.
 */
haeun_eval_result haeun_eval_cost_compute(const haeun_eval_run_metrics *m,
                                          const haeun_eval_cost_weights *w,
                                          haeun_eval_cost_result *out) {
    double ready_n;
    double score;
    uint32_t mn = 0u;
    uint32_t mx = 0u;
    unsigned i;
    int have = 0;
    if (m == NULL || w == NULL || out == NULL)
        return HAEUN_EVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->experimental = 1;
    out->weights = *w;
    if (w->w_ready_wait < 0.0 || w->w_ctx_ratio < 0.0 || w->w_imbalance < 0.0 ||
        w->w_incomplete < 0.0) {
        opt_na(&out->score, "negative weight rejected");
        return HAEUN_EVAL_BAD_ARGUMENT;
    }
    if (!m->ok) {
        opt_na(&out->score, "run metrics not ok");
        return HAEUN_EVAL_OK;
    }
    /* Normalize ready wait by participants*1000 when possible. */
    if (m->participants == 0u)
        opt_na(&out->s_ready, "no participants");
    else {
        ready_n = (double)m->total_ready_wait /
                  ((double)m->participants * 1000.0);
        opt_set(&out->s_ready, ready_n);
    }
    if (!m->ctx_dispatch_ratio.available)
        opt_na(&out->s_ctx, m->ctx_dispatch_ratio.reason);
    else
        opt_set(&out->s_ctx, m->ctx_dispatch_ratio.value);
    for (i = 0; i < HAEUN_TELEM_NPROC; ++i) {
        if (!m->slot_included[i])
            continue;
        if (!have || m->slot_run[i] < mn)
            mn = m->slot_run[i];
        if (!have || m->slot_run[i] > mx)
            mx = m->slot_run[i];
        have = 1;
    }
    if (!have)
        opt_na(&out->s_imbalance, "no participants");
    else if (mx == 0u)
        opt_set(&out->s_imbalance, 0.0);
    else
        opt_set(&out->s_imbalance, (double)(mx - mn) / (double)mx);
    opt_set(&out->s_incomplete, (double)m->incomplete);
    if (!out->s_ready.available || !out->s_ctx.available ||
        !out->s_imbalance.available) {
        opt_na(&out->score, "required component unavailable");
        return HAEUN_EVAL_OK;
    }
    score = w->w_ready_wait * out->s_ready.value +
            w->w_ctx_ratio * out->s_ctx.value +
            w->w_imbalance * out->s_imbalance.value +
            w->w_incomplete * out->s_incomplete.value;
    opt_set(&out->score, score);
    return HAEUN_EVAL_OK;
}
