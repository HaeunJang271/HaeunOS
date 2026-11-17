#ifndef HAEUN_EVAL_RUNNER_H
#define HAEUN_EVAL_RUNNER_H

#include "haeun/eval_types.h"

/**
 * Runs one workload repetition. policy_id selects image root (0=build_dir,
 * 1=build_dir/fcfs). Does not write guest scheduler state after load.
 */
haeun_eval_result haeun_eval_run_once(const haeun_eval_workload *wl,
                                      const char *build_dir, uint32_t policy_id,
                                      haeun_eval_run_metrics *out);

/**
 * Executes requested_reps independent runs and fills the experiment.
 * Default reps = HAEUN_EVAL_DEFAULT_REPS when caller passes 0.
 */
haeun_eval_result haeun_eval_experiment_run(const haeun_eval_workload *wl,
                                            const char *build_dir,
                                            uint32_t policy_id,
                                            const char *experiment_id,
                                            unsigned requested_reps,
                                            haeun_eval_experiment *out);

#endif
