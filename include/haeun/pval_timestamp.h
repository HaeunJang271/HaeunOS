#ifndef HAEUN_PVAL_TIMESTAMP_H
#define HAEUN_PVAL_TIMESTAMP_H

#include "haeun/pval_types.h"

typedef struct haeun_pval_timestamp_report {
    unsigned eligible_count;
    int pairwise_comparable;
    int exact_half_range_pair;
    int external_bound_proven;
    haeun_pval_check checks[8];
    unsigned check_count;
    haeun_pval_status overall;
} haeun_pval_timestamp_report;

/**
 * Validates ready_since pairwise modular ordering assumptions for FIFO.
 */
haeun_pval_result haeun_pval_timestamp_check(const haeun_pval_snapshot *snap,
                                             haeun_pval_timestamp_report *out);

#endif
