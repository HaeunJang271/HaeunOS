#include "haeun/planner.h"

#include <string.h>

static const char *dec_name(haeun_plan_decision d) {
    switch (d) {
    case HAEUN_PLAN_KEEP_CURRENT:
        return "KEEP_CURRENT";
    case HAEUN_PLAN_RECOMMEND_CHANGE:
        return "RECOMMEND_CHANGE";
    case HAEUN_PLAN_NO_DECISION:
        return "NO_DECISION";
    case HAEUN_PLAN_REJECTED_BY_SAFETY:
        return "REJECTED_BY_SAFETY";
    default:
        return "UNKNOWN";
    }
}

static const char *wl_name(haeun_plan_wl_class c) {
    switch (c) {
    case HAEUN_PLAN_WL_CPU_BOUND:
        return "CPU_BOUND";
    case HAEUN_PLAN_WL_SHORT_JOB:
        return "SHORT_JOB";
    case HAEUN_PLAN_WL_MIXED:
        return "MIXED";
    case HAEUN_PLAN_WL_BURSTY:
        return "BURSTY";
    case HAEUN_PLAN_WL_BLOCK_SLEEP:
        return "BLOCK_SLEEP";
    case HAEUN_PLAN_WL_BLOCK_WAIT:
        return "BLOCK_WAIT";
    case HAEUN_PLAN_WL_IDLE:
        return "IDLE";
    default:
        return "UNKNOWN";
    }
}

static const char *conf_name(haeun_plan_confidence c) {
    switch (c) {
    case HAEUN_PLAN_CONF_LOW:
        return "LOW";
    case HAEUN_PLAN_CONF_MEDIUM:
        return "MEDIUM";
    case HAEUN_PLAN_CONF_HIGH:
        return "HIGH";
    default:
        return "NONE";
    }
}

static const char *ev_kind(haeun_plan_ev_kind k) {
    switch (k) {
    case HAEUN_PLAN_EV_OK:
        return "OK";
    case HAEUN_PLAN_EV_ZERO:
        return "ZERO";
    case HAEUN_PLAN_EV_MISSING:
        return "MISSING";
    case HAEUN_PLAN_EV_INVALID:
        return "INVALID";
    case HAEUN_PLAN_EV_UNKNOWN:
        return "UNKNOWN";
    case HAEUN_PLAN_EV_NA:
        return "NA";
    default:
        return "?";
    }
}

/**
 * Human-readable plan report.
 */
void haeun_plan_fprint(FILE *fp, const haeun_plan *plan) {
    unsigned i;
    if (fp == NULL || plan == NULL)
        return;
    fprintf(fp, "HaeunOS Autonomous Policy Planner (R3.3)\n");
    fprintf(fp, "plan_id: %s\n", plan->plan_id);
    fprintf(fp, "mode: %s\n", plan->shadow_mode ? "SHADOW" : "EXECUTE");
    fprintf(fp, "decision: %s\n", dec_name(plan->decision));
    fprintf(fp, "source->target: %u -> %u\n", plan->source_policy,
            plan->target_policy);
    fprintf(fp, "workload: %s\n", wl_name(plan->workload_class));
    fprintf(fp, "confidence: %s (evidence-strength; not ML)\n",
            conf_name(plan->confidence));
    fprintf(fp, "reason: %s\n", plan->reason);
    fprintf(fp, "stability: %s\n", plan->stability_status);
    fprintf(fp, "safety: %s\n", plan->safety_status);
    if (plan->rejection_reason[0])
        fprintf(fp, "rejection: %s\n", plan->rejection_reason);
    fprintf(fp, "txn_attempted: %d committed: %d id: %s\n", plan->txn_attempted,
            plan->txn_committed, plan->txn_id[0] ? plan->txn_id : "-");
    fprintf(fp, "evidence:\n");
    for (i = 0; i < plan->evidence_count; ++i)
        fprintf(fp, "  %s kind=%s value=%g (%s)\n", plan->evidence[i].name,
                ev_kind(plan->evidence[i].kind), plan->evidence[i].value,
                plan->evidence[i].note);
    fprintf(fp, "disclaimer: rule-based planner; guest ticks≠wall-clock; "
                "R3.1 gates execution; shadow never writes 0xA474\n");
}

/**
 * JSON plan report with schema_version.
 */
void haeun_plan_fprint_json(FILE *fp, const haeun_plan *plan) {
    unsigned i;
    if (fp == NULL || plan == NULL)
        return;
    fprintf(fp,
            "{\"schema_version\":%d,\"plan_id\":\"%s\",\"shadow_mode\":%s,"
            "\"decision\":\"%s\",\"source_policy\":%u,\"target_policy\":%u,"
            "\"workload_class\":\"%s\",\"confidence\":\"%s\","
            "\"reason\":\"%s\",\"stability_status\":\"%s\","
            "\"safety_status\":\"%s\",\"rejection_reason\":",
            HAEUN_PLAN_SCHEMA, plan->plan_id,
            plan->shadow_mode ? "true" : "false", dec_name(plan->decision),
            plan->source_policy, plan->target_policy, wl_name(plan->workload_class),
            conf_name(plan->confidence), plan->reason, plan->stability_status,
            plan->safety_status);
    if (plan->rejection_reason[0])
        fprintf(fp, "\"%s\",", plan->rejection_reason);
    else
        fprintf(fp, "null,");
    fprintf(fp,
            "\"txn_attempted\":%s,\"txn_committed\":%s,\"evidence\":[",
            plan->txn_attempted ? "true" : "false",
            plan->txn_committed ? "true" : "false");
    for (i = 0; i < plan->evidence_count; ++i) {
        if (i)
            fputc(',', fp);
        fprintf(fp,
                "{\"name\":\"%s\",\"kind\":\"%s\",\"value\":%g,\"note\":\"%s\"}",
                plan->evidence[i].name, ev_kind(plan->evidence[i].kind),
                plan->evidence[i].value, plan->evidence[i].note);
    }
    fprintf(fp, "],\"disclaimer\":\"deterministic rule-based; not ML\"}");
}
