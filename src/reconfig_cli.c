#include "haeun/reconfig_cli.h"

#include "haeun/reconfig.h"

#include <stdio.h>
#include <string.h>

/**
 * Parses policy name "rr" or "fcfs". Returns -1 if unknown.
 */
int haeun_recfg_cli_parse_policy(const char *name, uint32_t *out) {
    if (name == NULL || out == NULL)
        return -1;
    if (strcmp(name, "rr") == 0) {
        *out = HAEUN_PVAL_POLICY_RR;
        return 0;
    }
    if (strcmp(name, "fcfs") == 0) {
        *out = HAEUN_PVAL_POLICY_FCFS;
        return 0;
    }
    return -1;
}

/**
 * Runs request→prepare→commit for source→target on a live VM.
 */
int haeun_recfg_cli_run(haeun_vm *vm, uint32_t source_policy,
                        uint32_t target_policy, int json) {
    haeun_recfg_txn txn;
    haeun_recfg_result rr;
    if (vm == NULL)
        return 2;
    haeun_recfg_fi_clear();
    rr = haeun_recfg_request(&txn, vm, source_policy, target_policy);
    if (rr != HAEUN_RECFG_OK) {
        if (json)
            haeun_recfg_report_fprint_json(stdout, &txn);
        else
            haeun_recfg_report_fprint(stdout, &txn);
        return 1;
    }
    rr = haeun_recfg_prepare(&txn, vm);
    if (rr != HAEUN_RECFG_OK) {
        if (json)
            haeun_recfg_report_fprint_json(stdout, &txn);
        else
            haeun_recfg_report_fprint(stdout, &txn);
        return 1;
    }
    rr = haeun_recfg_commit(&txn, vm);
    if (rr == HAEUN_RECFG_FAILED) {
        (void)haeun_recfg_rollback(&txn, vm);
    }
    if (json)
        haeun_recfg_report_fprint_json(stdout, &txn);
    else
        haeun_recfg_report_fprint(stdout, &txn);
    return (txn.state == HAEUN_RECFG_ST_COMMITTED) ? 0 : 1;
}
