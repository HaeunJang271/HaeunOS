#include "haeun/feedback.h"

#include <stdio.h>

static const char *outcome_name(haeun_fb_outcome o) {
    switch (o) {
    case HAEUN_FB_IMPROVED:
        return "IMPROVED";
    case HAEUN_FB_DEGRADED:
        return "DEGRADED";
    case HAEUN_FB_NEUTRAL:
        return "NEUTRAL";
    default:
        return "INCONCLUSIVE";
    }
}

static const char *mode_name(haeun_fb_mode m) {
    return m == HAEUN_FB_MODE_ADAPTIVE ? "ADAPTIVE" : "SHADOW";
}

/**
 * Human-readable feedback report.
 */
void haeun_fb_fprint(FILE *fp, const haeun_fb_state *st,
                     const haeun_fb_assessment *a, const haeun_plan *plan) {
    if (fp == NULL || st == NULL)
        return;
    fprintf(fp, "HaeunOS Adaptive Feedback Controller (R3.5)\n");
    fprintf(fp, "schema: %u controller: %s mode: %s cycle: %u\n",
            st->schema_version, st->controller_version, mode_name(st->mode),
            st->cycle_id);
    fprintf(fp, "policy current=%u previous=%u cooldown=%u\n",
            st->current_policy, st->previous_policy, st->cooldown_remaining);
    fprintf(fp, "streak improve=%u regress=%u\n", st->consecutive_improvements,
            st->consecutive_regressions);
    if (plan != NULL) {
        fprintf(fp, "plan decision=%d %u->%u shadow=%d committed=%d\n",
                (int)plan->decision, plan->source_policy, plan->target_policy,
                plan->shadow_mode, plan->txn_committed);
        fprintf(fp, "plan reason: %s\n", plan->reason);
    }
    if (a != NULL) {
        fprintf(fp, "feedback outcome: %s comparable=%d score_valid=%d\n",
                outcome_name(a->outcome), a->comparable, a->score_valid);
        fprintf(fp, "feedback reason: %s\n", a->reason);
        fprintf(fp, "limitations: %s\n", a->limitations);
    }
    fprintf(fp,
            "disclaimer: rule-based; guest ticks≠wall-clock; R3.1/R3.2/R3.4 "
            "remain authoritative; SHADOW default never commits\n");
}

/**
 * JSON report with schema_version.
 */
void haeun_fb_fprint_json(FILE *fp, const haeun_fb_state *st,
                          const haeun_fb_assessment *a, const haeun_plan *plan) {
    unsigned i;
    if (fp == NULL || st == NULL)
        return;
    fprintf(fp,
            "{\"schema_version\":%u,\"controller_version\":\"%s\",\"mode\":\"%s\","
            "\"cycle_id\":%u,\"current_policy\":%u,\"previous_policy\":%u,"
            "\"cooldown_remaining\":%u,\"consecutive_improvements\":%u,"
            "\"consecutive_regressions\":%u,",
            st->schema_version, st->controller_version, mode_name(st->mode),
            st->cycle_id, st->current_policy, st->previous_policy,
            st->cooldown_remaining, st->consecutive_improvements,
            st->consecutive_regressions);
    if (plan != NULL)
        fprintf(fp,
                "\"plan\":{\"decision\":%d,\"source_policy\":%u,"
                "\"target_policy\":%u,\"shadow_mode\":%s,\"txn_committed\":%s,"
                "\"reason\":\"%s\"},",
                (int)plan->decision, plan->source_policy, plan->target_policy,
                plan->shadow_mode ? "true" : "false",
                plan->txn_committed ? "true" : "false", plan->reason);
    if (a != NULL) {
        fprintf(fp,
                "\"assessment\":{\"outcome\":\"%s\",\"comparable\":%s,"
                "\"score_valid\":%s,\"weighted_score\":%.6f,\"reason\":\"%s\","
                "\"objectives\":[",
                outcome_name(a->outcome), a->comparable ? "true" : "false",
                a->score_valid ? "true" : "false", a->weighted_score, a->reason);
        for (i = 0; i < a->obj_count; ++i) {
            fprintf(fp,
                    "%s{\"metric\":\"%s\",\"validity\":%d,\"before\":%.6f,"
                    "\"after\":%.6f,\"delta\":%.6f,\"improved\":%s,"
                    "\"degraded\":%s}",
                    i ? "," : "", a->objs[i].metric_name,
                    (int)a->objs[i].validity, a->objs[i].before,
                    a->objs[i].after, a->objs[i].relative_delta,
                    a->objs[i].improved ? "true" : "false",
                    a->objs[i].degraded ? "true" : "false");
        }
        fprintf(fp, "]},");
    }
    fprintf(fp,
            "\"disclaimer\":\"deterministic rule-based feedback; not ML\"}\n");
}
