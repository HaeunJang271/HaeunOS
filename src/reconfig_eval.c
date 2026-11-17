#include "haeun/reconfig_eval.h"

#include <string.h>

/**
 * Clears evaluation counters.
 */
void haeun_recfg_eval_clear(haeun_recfg_eval *ev) {
    if (ev == NULL)
        return;
    memset(ev, 0, sizeof(*ev));
}

/**
 * Records one finished transaction into evaluation counters.
 */
void haeun_recfg_eval_record(haeun_recfg_eval *ev, const haeun_recfg_txn *txn,
                             uint64_t validation_host_ns,
                             uint64_t commit_host_ns,
                             uint64_t guest_ticks_delta,
                             int guest_ticks_ok) {
    if (ev == NULL || txn == NULL)
        return;
    ev->validation_host_ns_total += validation_host_ns;
    ev->commit_host_ns_total += commit_host_ns;
    if (guest_ticks_ok) {
        ev->guest_ticks_measurable = 1;
        ev->commit_guest_ticks_delta_total += guest_ticks_delta;
    }
    switch (txn->state) {
    case HAEUN_RECFG_ST_COMMITTED:
        ev->success_count++;
        break;
    case HAEUN_RECFG_ST_REJECTED:
        ev->rejected_count++;
        break;
    case HAEUN_RECFG_ST_ROLLED_BACK:
        ev->rollback_count++;
        ev->rollback_ok_count++;
        break;
    case HAEUN_RECFG_ST_FAILED:
        ev->failed_count++;
        if (txn->reject_reason[0] == '\0' ||
            strstr(txn->reject_reason, "rollback") != NULL)
            ev->rollback_count++;
        break;
    default:
        break;
    }
    if (txn->validation_ran &&
        txn->validation.invariants.overall == HAEUN_PVAL_FAIL)
        ev->invariant_violations++;
}

/**
 * Prints human-readable evaluation summary.
 */
void haeun_recfg_eval_fprint(FILE *fp, const haeun_recfg_eval *ev) {
    if (fp == NULL || ev == NULL)
        return;
    fprintf(fp, "R3.2 reconfiguration evaluation\n");
    fprintf(fp, "success: %u\n", ev->success_count);
    fprintf(fp, "rejected: %u\n", ev->rejected_count);
    fprintf(fp, "rollback attempts: %u (ok %u)\n", ev->rollback_count,
            ev->rollback_ok_count);
    fprintf(fp, "failed: %u\n", ev->failed_count);
    fprintf(fp, "invariant FAIL sightings: %u\n", ev->invariant_violations);
    fprintf(fp, "validation host ns total: %llu\n",
            (unsigned long long)ev->validation_host_ns_total);
    fprintf(fp, "commit host ns total: %llu\n",
            (unsigned long long)ev->commit_host_ns_total);
    if (ev->guest_ticks_measurable)
        fprintf(fp, "commit guest ticks delta total: %llu\n",
                (unsigned long long)ev->commit_guest_ticks_delta_total);
    else
        fprintf(fp, "commit guest ticks: not measured\n");
    fprintf(fp, "note: host ns and guest ticks are separate; not zero-cost\n");
}
