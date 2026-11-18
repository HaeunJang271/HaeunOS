#ifndef HAEUN_FEEDBACK_H
#define HAEUN_FEEDBACK_H

#include "haeun/feedback_types.h"
#include "haeun/vm.h"

#include <stdio.h>

/**
 * Fills conservative multi-objective defaults (documented experimental).
 */
void haeun_fb_config_default(haeun_fb_config *cfg);

/**
 * Initializes controller state (SHADOW mode).
 */
void haeun_fb_state_init(haeun_fb_state *st);

/**
 * Builds a feedback sample from R2.2 run metrics. Missing stays UNAVAILABLE.
 */
haeun_fb_result haeun_fb_sample_from_metrics(const haeun_eval_run_metrics *m,
                                             uint32_t policy_id,
                                             uint64_t timer_count,
                                             const char *workload_id,
                                             uint32_t fingerprint,
                                             haeun_fb_sample *out);

/**
 * Samples live VM telemetry into a feedback sample.
 */
haeun_fb_result haeun_fb_sample_from_vm(const haeun_vm *vm,
                                        const char *workload_id,
                                        uint32_t fingerprint,
                                        haeun_fb_sample *out);

/**
 * Returns 1 when before/after are comparable under documented rules.
 */
int haeun_fb_comparable(const haeun_fb_sample *before,
                        const haeun_fb_sample *after, char *why, size_t why_n);

/**
 * Multi-objective before/after assessment (deterministic).
 */
haeun_fb_result haeun_fb_evaluate(const haeun_fb_config *cfg,
                                  const haeun_fb_sample *before,
                                  const haeun_fb_sample *after,
                                  haeun_fb_assessment *out);

/**
 * Applies feedback gates to a planner recommendation (does not execute).
 */
void haeun_fb_apply_to_plan(const haeun_fb_state *st, haeun_plan *plan);

/**
 * Records assessment into controller history/stability counters.
 */
void haeun_fb_record_outcome(haeun_fb_state *st, const haeun_plan *plan,
                             const haeun_fb_assessment *a, int txn_committed);

/**
 * One adaptive cycle. shadow_or_dry never commits. adaptive+execute may.
 */
haeun_fb_result haeun_fb_cycle(haeun_fb_state *st, haeun_plan_session *psess,
                               haeun_vm *vm, int execute, haeun_plan *plan_out,
                               haeun_fb_assessment *assess_out);

/**
 * Saves state as deterministic JSON. Rejects NULL.
 */
haeun_fb_result haeun_fb_state_save(FILE *fp, const haeun_fb_state *st);

/**
 * Loads state; incompatible schema → BAD_STATE.
 */
haeun_fb_result haeun_fb_state_load(FILE *fp, haeun_fb_state *st);

/**
 * Validates loaded state invariants.
 */
haeun_fb_result haeun_fb_state_validate(const haeun_fb_state *st);

/**
 * Human-readable feedback report.
 */
void haeun_fb_fprint(FILE *fp, const haeun_fb_state *st,
                     const haeun_fb_assessment *a, const haeun_plan *plan);

/**
 * JSON report with schema_version.
 */
void haeun_fb_fprint_json(FILE *fp, const haeun_fb_state *st,
                          const haeun_fb_assessment *a, const haeun_plan *plan);

/**
 * CLI entry: report / adaptive cycles / dry-run / state file.
 * Returns process exit code (0 ok, 1 fail, 2 usage).
 */
int haeun_fb_cli_run(haeun_vm *vm, int json, int adaptive, int dry_run,
                     unsigned cycles, const char *state_path);

#endif
