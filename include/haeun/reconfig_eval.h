#ifndef HAEUN_RECONFIG_EVAL_H
#define HAEUN_RECONFIG_EVAL_H

#include "haeun/reconfig.h"

#include <stdio.h>
#include <stdint.h>

/** Host-side R3.2 reconfiguration counters (not guest ticks). */
typedef struct haeun_recfg_eval {
    uint32_t success_count;
    uint32_t rejected_count;
    uint32_t rollback_count;
    uint32_t rollback_ok_count;
    uint32_t failed_count;
    uint32_t invariant_violations;
    uint64_t validation_host_ns_total;
    uint64_t commit_host_ns_total;
    uint64_t commit_guest_ticks_delta_total;
    int guest_ticks_measurable;
} haeun_recfg_eval;

/**
 * Clears evaluation counters.
 */
void haeun_recfg_eval_clear(haeun_recfg_eval *ev);

/**
 * Records one finished transaction into evaluation counters.
 */
void haeun_recfg_eval_record(haeun_recfg_eval *ev, const haeun_recfg_txn *txn,
                             uint64_t validation_host_ns,
                             uint64_t commit_host_ns,
                             uint64_t guest_ticks_delta,
                             int guest_ticks_ok);

/**
 * Prints human-readable evaluation summary.
 */
void haeun_recfg_eval_fprint(FILE *fp, const haeun_recfg_eval *ev);

#endif
