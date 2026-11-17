#ifndef HAEUN_EVAL_METRICS_H
#define HAEUN_EVAL_METRICS_H

#include "haeun/eval_types.h"
#include "haeun/telem_inspect.h"
#include "haeun/vm.h"

/**
 * Builds run metrics from a telemetry snapshot.
 * Excludes slot 3 and FREE slots with no completion evidence.
 */
haeun_eval_result haeun_eval_metrics_from_telem(const haeun_telem_report *rep,
                                                uint32_t exit_code,
                                                uint64_t steps,
                                                haeun_eval_run_metrics *out);

/**
 * Reads guest telemetry then fills metrics. Does not write guest RAM.
 */
haeun_eval_result haeun_eval_metrics_from_vm(const haeun_vm *vm, uint32_t exit_code,
                                             uint64_t steps,
                                             haeun_eval_run_metrics *out);

#endif
