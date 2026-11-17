#ifndef HAEUN_PVAL_INVARIANTS_H
#define HAEUN_PVAL_INVARIANTS_H

#include "haeun/pval_types.h"

typedef struct haeun_pval_invariant_report {
    unsigned count;
    haeun_pval_check checks[HAEUN_PVAL_CHECK_MAX];
    haeun_pval_status overall;
} haeun_pval_invariant_report;

/**
 * Runs PCB/current/policy/telemetry invariant checks on a snapshot.
 */
haeun_pval_result haeun_pval_invariants_check(const haeun_pval_snapshot *snap,
                                              haeun_pval_invariant_report *out);

#endif
