#include "haeun/feedback.h"

#include <stdio.h>
#include <string.h>

/**
 * Returns 1 when before/after are comparable under documented rules.
 */
int haeun_fb_comparable(const haeun_fb_sample *before,
                        const haeun_fb_sample *after, char *why, size_t why_n) {
    if (before == NULL || after == NULL) {
        if (why != NULL && why_n > 0u)
            snprintf(why, why_n, "null sample");
        return 0;
    }
    if (!before->ok || !after->ok) {
        if (why != NULL && why_n > 0u)
            snprintf(why, why_n, "sample not ok");
        return 0;
    }
    if (before->fingerprint != after->fingerprint) {
        if (why != NULL && why_n > 0u)
            snprintf(why, why_n, "workload fingerprint mismatch");
        return 0;
    }
    if (strcmp(before->workload_id, after->workload_id) != 0) {
        if (why != NULL && why_n > 0u)
            snprintf(why, why_n, "workload_id mismatch");
        return 0;
    }
    if (before->participants != after->participants) {
        if (why != NULL && why_n > 0u)
            snprintf(why, why_n, "participant count differs (%u vs %u)",
                     before->participants, after->participants);
        return 0;
    }
    if (why != NULL && why_n > 0u)
        snprintf(why, why_n, "comparable");
    return 1;
}

static const haeun_fb_metric *find_metric(const haeun_fb_sample *s,
                                         const char *name) {
    if (strcmp(name, "avg_ready_wait") == 0)
        return &s->avg_ready_wait;
    if (strcmp(name, "jain_fairness") == 0)
        return &s->jain_fairness;
    if (strcmp(name, "dispatch_count") == 0)
        return &s->dispatch_count;
    if (strcmp(name, "completed") == 0)
        return &s->completed;
    if (strcmp(name, "max_ready_wait") == 0)
        return &s->max_ready_wait;
    if (strcmp(name, "ctx_switches") == 0)
        return &s->ctx_switches;
    if (strcmp(name, "total_run") == 0)
        return &s->total_run;
    if (strcmp(name, "completion_latency") == 0)
        return &s->completion_latency;
    return NULL;
}

static void eval_one(const haeun_fb_objective *obj, const haeun_fb_sample *b,
                     const haeun_fb_sample *a, haeun_fb_obj_result *r) {
    const haeun_fb_metric *mb;
    const haeun_fb_metric *ma;
    double denom;
    memset(r, 0, sizeof(*r));
    snprintf(r->metric_name, sizeof(r->metric_name), "%s", obj->metric_name);
    mb = find_metric(b, obj->metric_name);
    ma = find_metric(a, obj->metric_name);
    if (mb == NULL || ma == NULL) {
        r->validity = HAEUN_FB_UNAVAILABLE;
        snprintf(r->note, sizeof(r->note), "unknown metric");
        return;
    }
    if (mb->validity != HAEUN_FB_VALID || ma->validity != HAEUN_FB_VALID) {
        r->validity = (mb->validity != HAEUN_FB_VALID) ? mb->validity
                                                       : ma->validity;
        snprintf(r->note, sizeof(r->note), "metric not VALID both sides");
        return;
    }
    if (b->participants < obj->min_participants ||
        a->participants < obj->min_participants) {
        r->validity = HAEUN_FB_INSUFFICIENT_SAMPLE;
        snprintf(r->note, sizeof(r->note), "participants below min");
        return;
    }
    r->validity = HAEUN_FB_VALID;
    r->before = mb->value;
    r->after = ma->value;
    denom = (mb->value < 0.0 ? -mb->value : mb->value);
    if (denom < 1e-12)
        denom = 1.0;
    r->relative_delta = (ma->value - mb->value) / denom;
    if (obj->direction == HAEUN_FB_DIR_LOWER_BETTER) {
        if (ma->value < mb->value &&
            (-r->relative_delta) >= obj->improve_threshold)
            r->improved = 1;
        if (ma->value > mb->value &&
            r->relative_delta >= obj->regress_threshold)
            r->degraded = 1;
    } else {
        if (ma->value > mb->value &&
            r->relative_delta >= obj->improve_threshold)
            r->improved = 1;
        if (ma->value < mb->value &&
            (-r->relative_delta) >= obj->regress_threshold)
            r->degraded = 1;
    }
    snprintf(r->note, sizeof(r->note), "delta=%.4f", r->relative_delta);
}

/**
 * Multi-objective before/after assessment (deterministic).
 */
haeun_fb_result haeun_fb_evaluate(const haeun_fb_config *cfg,
                                  const haeun_fb_sample *before,
                                  const haeun_fb_sample *after,
                                  haeun_fb_assessment *out) {
    char why[HAEUN_FB_MSG_LEN];
    unsigned i;
    unsigned valid = 0u;
    unsigned imp = 0u;
    unsigned deg = 0u;
    double score = 0.0;
    if (cfg == NULL || before == NULL || after == NULL || out == NULL)
        return HAEUN_FB_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->comparable = haeun_fb_comparable(before, after, why, sizeof(why));
    snprintf(out->limitations, sizeof(out->limitations),
             "guest ticks≠wall-clock; experimental weights");
    if (!out->comparable) {
        out->outcome = HAEUN_FB_INCONCLUSIVE;
        snprintf(out->reason, sizeof(out->reason), "incomparable");
        snprintf(out->limitations, sizeof(out->limitations), "%s", why);
        return HAEUN_FB_INCOMPARABLE;
    }
    out->obj_count = cfg->obj_count;
    for (i = 0; i < cfg->obj_count && i < HAEUN_FB_OBJ_MAX; ++i) {
        eval_one(&cfg->objs[i], before, after, &out->objs[i]);
        if (out->objs[i].validity != HAEUN_FB_VALID)
            continue;
        valid++;
        if (out->objs[i].improved) {
            imp++;
            score += cfg->objs[i].weight;
        }
        if (out->objs[i].degraded) {
            deg++;
            score -= cfg->objs[i].weight;
        }
    }
    out->weighted_score = score;
    out->score_valid = (valid >= cfg->min_valid_objectives) ? 1 : 0;
    if (!out->score_valid) {
        out->outcome = HAEUN_FB_INCONCLUSIVE;
        snprintf(out->reason, sizeof(out->reason),
                 "insufficient valid objectives (%u < %u)", valid,
                 cfg->min_valid_objectives);
        return HAEUN_FB_OK;
    }
    if (imp > 0u && deg > 0u) {
        out->outcome = HAEUN_FB_INCONCLUSIVE;
        snprintf(out->reason, sizeof(out->reason),
                 "conflicting objectives imp=%u deg=%u score=%.3f", imp, deg,
                 score);
        return HAEUN_FB_OK;
    }
    if (score > 1e-9 && deg == 0u) {
        out->outcome = HAEUN_FB_IMPROVED;
        snprintf(out->reason, sizeof(out->reason), "net improvement score=%.3f",
                 score);
    } else if (score < -1e-9 && imp == 0u) {
        out->outcome = HAEUN_FB_DEGRADED;
        snprintf(out->reason, sizeof(out->reason), "net regression score=%.3f",
                 score);
    } else {
        out->outcome = HAEUN_FB_NEUTRAL;
        snprintf(out->reason, sizeof(out->reason),
                 "within thresholds score=%.3f", score);
    }
    return HAEUN_FB_OK;
}
