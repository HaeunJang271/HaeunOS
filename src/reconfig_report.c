#include "haeun/reconfig.h"

#include <string.h>

static const char *st_name(haeun_recfg_state s) {
    switch (s) {
    case HAEUN_RECFG_ST_IDLE:
        return "IDLE";
    case HAEUN_RECFG_ST_REQUESTED:
        return "REQUESTED";
    case HAEUN_RECFG_ST_PREFLIGHT:
        return "PREFLIGHT";
    case HAEUN_RECFG_ST_SNAPSHOT:
        return "SNAPSHOT_CAPTURED";
    case HAEUN_RECFG_ST_VALIDATING:
        return "VALIDATING";
    case HAEUN_RECFG_ST_PREPARED:
        return "PREPARED";
    case HAEUN_RECFG_ST_COMMITTING:
        return "COMMITTING";
    case HAEUN_RECFG_ST_COMMITTED:
        return "COMMITTED";
    case HAEUN_RECFG_ST_VERIFYING:
        return "VERIFYING";
    case HAEUN_RECFG_ST_ROLLING_BACK:
        return "ROLLING_BACK";
    case HAEUN_RECFG_ST_ROLLED_BACK:
        return "ROLLED_BACK";
    case HAEUN_RECFG_ST_REJECTED:
        return "REJECTED";
    case HAEUN_RECFG_ST_FAILED:
        return "FAILED";
    default:
        return "UNKNOWN";
    }
}

/**
 * Prints human-readable transaction report.
 */
void haeun_recfg_report_fprint(FILE *fp, const haeun_recfg_txn *txn) {
    unsigned i;
    if (fp == NULL || txn == NULL)
        return;
    fprintf(fp, "HaeunOS Runtime Reconfiguration (R3.2)\n");
    fprintf(fp, "txn_id: %s\n", txn->txn_id);
    fprintf(fp, "state: %s\n", st_name(txn->state));
    fprintf(fp, "source->target: %u -> %u\n", txn->source_policy,
            txn->target_policy);
    fprintf(fp, "observed_at_request: %u\n", txn->observed_policy_at_request);
    if (txn->reject_reason[0])
        fprintf(fp, "reject_reason: %s\n", txn->reject_reason);
    fprintf(fp, "policy_id_restored: %s\n",
            txn->policy_id_restored ? "yes" : "no");
    fprintf(fp, "rollback_scope: policy ID word only (not full VM)\n");
    fprintf(fp, "commit_model: serialized between haeun_vm_step calls\n");
    fprintf(fp, "log:\n");
    for (i = 0; i < txn->log_count; ++i)
        fprintf(fp, "  [%s] %s\n", st_name(txn->log[i].state),
                txn->log[i].message);
}

/**
 * Prints JSON transaction report.
 */
void haeun_recfg_report_fprint_json(FILE *fp, const haeun_recfg_txn *txn) {
    unsigned i;
    if (fp == NULL || txn == NULL)
        return;
    fprintf(fp,
            "{\"schema_version\":1,\"txn_id\":\"%s\",\"state\":\"%s\","
            "\"source_policy\":%u,\"target_policy\":%u,"
            "\"observed_policy_at_request\":%u,\"policy_id_restored\":%s,"
            "\"reject_reason\":",
            txn->txn_id, st_name(txn->state), txn->source_policy,
            txn->target_policy, txn->observed_policy_at_request,
            txn->policy_id_restored ? "true" : "false");
    if (txn->reject_reason[0])
        fprintf(fp, "\"%s\",", txn->reject_reason);
    else
        fprintf(fp, "null,");
    fprintf(fp, "\"log\":[");
    for (i = 0; i < txn->log_count; ++i) {
        if (i)
            fputc(',', fp);
        fprintf(fp, "{\"state\":\"%s\",\"message\":\"%s\"}",
                st_name(txn->log[i].state), txn->log[i].message);
    }
    fprintf(fp,
            "],\"disclaimer\":\"serialized policy-ID commit; not SMP-atomic;"
            " UNKNOWN validation rejects\"}");
}
