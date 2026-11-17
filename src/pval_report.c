#include "haeun/pval_report.h"

#include <string.h>

static const char *st_name(haeun_pval_status s) {
    switch (s) {
    case HAEUN_PVAL_PASS:
        return "PASS";
    case HAEUN_PVAL_FAIL:
        return "FAIL";
    case HAEUN_PVAL_WARN:
        return "WARN";
    case HAEUN_PVAL_UNKNOWN:
        return "UNKNOWN";
    case HAEUN_PVAL_NOT_APPLICABLE:
        return "NOT_APPLICABLE";
    default:
        return "INVALID";
    }
}

/**
 * Aggregates overall status: FAIL > UNKNOWN > WARN > PASS (N/A ignored).
 */
haeun_pval_status haeun_pval_aggregate(const haeun_pval_status *items,
                                       unsigned n) {
    unsigned i;
    haeun_pval_status o = HAEUN_PVAL_PASS;
    int any = 0;
    if (items == NULL)
        return HAEUN_PVAL_UNKNOWN;
    for (i = 0; i < n; ++i) {
        haeun_pval_status s = items[i];
        if (s == HAEUN_PVAL_NOT_APPLICABLE)
            continue;
        any = 1;
        if (s == HAEUN_PVAL_FAIL)
            return HAEUN_PVAL_FAIL;
        if (s == HAEUN_PVAL_UNKNOWN)
            o = HAEUN_PVAL_UNKNOWN;
        else if (s == HAEUN_PVAL_WARN && o == HAEUN_PVAL_PASS)
            o = HAEUN_PVAL_WARN;
    }
    return any ? o : HAEUN_PVAL_NOT_APPLICABLE;
}

static void emit_checks(FILE *fp, const haeun_pval_check *c, unsigned n) {
    unsigned i;
    for (i = 0; i < n; ++i)
        fprintf(fp, "  [%s] %s — %s\n", st_name(c[i].status), c[i].id,
                c[i].explanation);
}

/**
 * Prints a human-readable validation report.
 */
void haeun_pval_report_fprint(FILE *fp, const haeun_pval_report *rep) {
    if (fp == NULL || rep == NULL)
        return;
    fprintf(fp, "HaeunOS Policy Validation (R3.1)\n");
    fprintf(fp, "validation_id: %s\n", rep->validation_id);
    fprintf(fp, "active_policy: %u\n", rep->snap.policy_id);
    fprintf(fp, "read_only_audit: %s\n", st_name(rep->read_only));
    fprintf(fp, "invariants: %s\n", st_name(rep->invariants.overall));
    emit_checks(fp, rep->invariants.checks, rep->invariants.count);
    fprintf(fp, "timestamp_validation: %s (eligible=%u)\n",
            st_name(rep->timestamps.overall), rep->timestamps.eligible_count);
    emit_checks(fp, rep->timestamps.checks, rep->timestamps.check_count);
    if (rep->compat_ran) {
        fprintf(fp, "compatibility: %s source=%u target=%u preconditions=%s\n",
                st_name(rep->compat.overall), rep->compat.source_policy,
                rep->compat.target_policy,
                rep->compat.transition_preconditions_satisfied ? "yes" : "no");
        emit_checks(fp, rep->compat.checks, rep->compat.check_count);
    }
    fprintf(fp, "overall_status: %s\n", st_name(rep->overall));
    fprintf(fp, "note: R3.1 does not switch policies; PASS≠transition_safe.\n");
}

static void json_checks(FILE *fp, const haeun_pval_check *c, unsigned n) {
    unsigned i;
    fprintf(fp, "[");
    for (i = 0; i < n; ++i) {
        if (i)
            fputc(',', fp);
        fprintf(fp,
                "{\"id\":\"%s\",\"status\":\"%s\",\"explanation\":\"%s\"}",
                c[i].id, st_name(c[i].status), c[i].explanation);
    }
    fprintf(fp, "]");
}

/**
 * Prints schema-versioned JSON.
 */
void haeun_pval_report_fprint_json(FILE *fp, const haeun_pval_report *rep) {
    if (fp == NULL || rep == NULL)
        return;
    fprintf(fp, "{\"schema_version\":%d,\"validation_id\":\"%s\",",
            HAEUN_PVAL_SCHEMA, rep->validation_id);
    fprintf(fp,
            "\"snapshot_metadata\":{\"policy_id\":%u,\"curr_index\":%u,"
            "\"idle_flag\":%u,\"eligible\":%u,\"participants\":%u},",
            rep->snap.policy_id, rep->snap.curr_index, rep->snap.idle_flag,
            rep->snap.ready_eligible_count, rep->snap.participant_count);
    fprintf(fp, "\"active_policy\":%u,", rep->snap.policy_id);
    if (rep->compat_ran)
        fprintf(fp, "\"target_policy\":%u,", rep->compat.target_policy);
    else
        fprintf(fp, "\"target_policy\":null,");
    fprintf(fp, "\"invariants\":{\"overall\":\"%s\",\"checks\":",
            st_name(rep->invariants.overall));
    json_checks(fp, rep->invariants.checks, rep->invariants.count);
    fprintf(fp, "},\"timestamp_validation\":{\"overall\":\"%s\","
                "\"pairwise_comparable\":%s,\"external_bound_proven\":%s,"
                "\"checks\":",
            st_name(rep->timestamps.overall),
            rep->timestamps.pairwise_comparable ? "true" : "false",
            rep->timestamps.external_bound_proven ? "true" : "false");
    json_checks(fp, rep->timestamps.checks, rep->timestamps.check_count);
    fprintf(fp, "},\"compatibility\":");
    if (rep->compat_ran) {
        fprintf(fp,
                "{\"overall\":\"%s\",\"transition_preconditions_satisfied\":%s,"
                "\"source_policy\":%u,\"target_policy\":%u,\"checks\":",
                st_name(rep->compat.overall),
                rep->compat.transition_preconditions_satisfied ? "true"
                                                               : "false",
                rep->compat.source_policy, rep->compat.target_policy);
        json_checks(fp, rep->compat.checks, rep->compat.check_count);
        fprintf(fp, "}");
    } else
        fprintf(fp, "null");
    fprintf(fp,
            ",\"read_only_audit\":\"%s\",\"overall_status\":\"%s\","
            "\"disclaimer\":\"validation-only; not a runtime transition\"}",
            st_name(rep->read_only), st_name(rep->overall));
}
