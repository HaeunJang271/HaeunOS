#ifndef HAEUN_PVAL_REPORT_H
#define HAEUN_PVAL_REPORT_H

#include "haeun/pval_compat.h"
#include "haeun/pval_invariants.h"
#include "haeun/pval_timestamp.h"
#include "haeun/pval_types.h"

#include <stdio.h>

typedef struct haeun_pval_report {
    char validation_id[HAEUN_PVAL_ID_LEN];
    haeun_pval_snapshot snap;
    haeun_pval_invariant_report invariants;
    haeun_pval_timestamp_report timestamps;
    haeun_pval_compat_report compat;
    int compat_ran;
    haeun_pval_status read_only;
    haeun_pval_status overall;
} haeun_pval_report;

/**
 * Aggregates overall status: FAIL > UNKNOWN > WARN > PASS (N/A ignored).
 */
haeun_pval_status haeun_pval_aggregate(const haeun_pval_status *items,
                                       unsigned n);

/**
 * Prints a human-readable validation report.
 */
void haeun_pval_report_fprint(FILE *fp, const haeun_pval_report *rep);

/**
 * Prints schema-versioned JSON.
 */
void haeun_pval_report_fprint_json(FILE *fp, const haeun_pval_report *rep);

#endif
