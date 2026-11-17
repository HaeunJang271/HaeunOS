#ifndef HAEUN_PVAL_COMPAT_H
#define HAEUN_PVAL_COMPAT_H

#include "haeun/pval_timestamp.h"
#include "haeun/pval_types.h"

typedef struct haeun_pval_compat_report {
    uint32_t source_policy;
    uint32_t target_policy;
    int transition_preconditions_satisfied;
    haeun_pval_check checks[16];
    unsigned check_count;
    haeun_pval_status overall;
} haeun_pval_compat_report;

/**
 * Evaluates transition preconditions without performing a transition.
 */
haeun_pval_result haeun_pval_compat_check(const haeun_pval_snapshot *snap,
                                          uint32_t target_policy,
                                          const haeun_pval_timestamp_report *ts,
                                          haeun_pval_compat_report *out);

#endif
