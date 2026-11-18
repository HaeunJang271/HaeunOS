#ifndef HAEUN_OPTIMIZE_H
#define HAEUN_OPTIMIZE_H

#include "haeun/opt_types.h"
#include "haeun/vm.h"

#include <stdio.h>

/**
 * Loads named objective profile (experimental defaults).
 */
haeun_opt_result haeun_opt_profile_get(haeun_opt_profile_id id,
                                       haeun_opt_profile *out);

/**
 * Parses profile name; returns -1 if unknown.
 */
int haeun_opt_profile_parse(const char *name, haeun_opt_profile_id *out);

/**
 * Default optimizer config (reps=3, balanced, SHADOW-friendly).
 */
void haeun_opt_config_default(haeun_opt_config *cfg);

/**
 * Initializes optimizer state in SHADOW mode.
 */
void haeun_opt_state_init(haeun_opt_state *st);

/**
 * Fills metric stats from sample array; missing tracked separately.
 */
haeun_opt_result haeun_opt_stats_fill(const double *vals, unsigned n,
                                      unsigned missing, const char *name,
                                      haeun_opt_metric_stats *out);

/**
 * Normalizes metric for scoring: lower-better → smaller is higher score in [0,1]
 * relative to (a,b) pair. Returns 0 if either unavailable.
 */
int haeun_opt_normalize_pair(double a, double b, haeun_opt_dir dir, double *na,
                             double *nb);

/**
 * Scores a candidate under profile using available means; sets score_valid.
 */
haeun_opt_result haeun_opt_score_candidate(const haeun_opt_profile *prof,
                                           haeun_opt_candidate *cand);

/**
 * Pairwise normalized scores for two candidates (preferred for RR vs FIFO).
 */
void haeun_opt_score_pair(const haeun_opt_profile *prof, haeun_opt_candidate *a,
                          haeun_opt_candidate *b);

/**
 * Marks Pareto non-dominated among candidates with score_valid metrics.
 */
void haeun_opt_pareto_mark(haeun_opt_candidate *cands, unsigned n);

/**
 * Selects recommendation for one workload result under profile.
 */
haeun_opt_result haeun_opt_select(const haeun_opt_profile *prof,
                                  haeun_opt_wl_result *wl);

/**
 * Runs RR+FIFO experiments for one workload into wl_result.
 */
haeun_opt_result haeun_opt_run_workload(const haeun_opt_config *cfg,
                                        const haeun_opt_profile *prof,
                                        haeun_eval_wl_kind kind,
                                        haeun_opt_wl_result *out);

/**
 * Runs optimization over all catalog workloads (OPTIMIZE mode; no APPLY).
 */
haeun_opt_result haeun_opt_run(haeun_opt_state *st);

/**
 * Saves/loads/validates versioned optimization state JSON.
 */
haeun_opt_result haeun_opt_state_save(FILE *fp, const haeun_opt_state *st);
haeun_opt_result haeun_opt_state_load(FILE *fp, haeun_opt_state *st);
haeun_opt_result haeun_opt_state_validate(const haeun_opt_state *st);

/**
 * Applies a recommendation via R3.1→R3.2 only (never writes policy directly).
 */
haeun_opt_result haeun_opt_apply(haeun_vm *vm,
                                 const haeun_opt_recommendation *rec,
                                 haeun_plan *plan_out);

/**
 * Text / JSON reports.
 */
void haeun_opt_fprint(FILE *fp, const haeun_opt_state *st);
void haeun_opt_fprint_json(FILE *fp, const haeun_opt_state *st);

/**
 * CLI: optimize / report / apply. Returns process exit code.
 */
int haeun_opt_cli_run(haeun_vm *vm, const char *build_dir, int json,
                      int optimize, int dry_run, int apply,
                      const char *profile_name, unsigned reps,
                      const char *state_path, const char *apply_path);

#endif
