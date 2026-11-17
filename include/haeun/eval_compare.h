#ifndef HAEUN_EVAL_COMPARE_H
#define HAEUN_EVAL_COMPARE_H

#include "haeun/eval_report.h"
#include "haeun/eval_types.h"

#include <stdio.h>

/**
 * Compares two RR experiment bundles under compatible workloads.
 * Does not claim policy superiority.
 */
haeun_eval_result haeun_eval_compare(const haeun_eval_report_bundle *a,
                                     const haeun_eval_report_bundle *b,
                                     FILE *fp);

#endif
