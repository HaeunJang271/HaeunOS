#ifndef HAEUN_ROBUST_H
#define HAEUN_ROBUST_H

#include "haeun/robust_types.h"
#include "haeun/vm.h"

#include <stdio.h>

/**
 * Initializes robustness state / default smoke preset.
 */
void haeun_rb_state_init(haeun_rb_state *st);

/**
 * Applies experiment preset (smoke/standard/extended/research).
 */
haeun_rb_result haeun_rb_preset_apply(haeun_rb_config *cfg,
                                      haeun_rb_preset preset);

/**
 * Parses preset name; -1 if unknown.
 */
int haeun_rb_preset_parse(const char *name, haeun_rb_preset *out);

/**
 * Deterministic FNV-1a seed for catalog workload+policy+step_cap.
 */
uint32_t haeun_rb_workload_seed(haeun_eval_wl_kind kind, uint32_t policy_id);

/**
 * Configuration hash over build_dir, reps, profile, preset.
 */
uint32_t haeun_rb_config_hash(const haeun_rb_config *cfg);

/**
 * Runs repeated RR/FIFO experiments + replay/reliability for all workloads.
 */
haeun_rb_result haeun_rb_run_experiments(haeun_rb_state *st);

/**
 * Compares two run fingerprints (ignores wall-clock; uses metrics).
 */
haeun_rb_replay_class haeun_rb_compare_fp(const haeun_rb_run_fp *a,
                                          const haeun_rb_run_fp *b);

/**
 * Classifies statistical reliability from candidate sample counts.
 */
haeun_rb_reliability haeun_rb_classify_reliability(const haeun_opt_candidate *c,
                                                   unsigned min_n);

/**
 * Sensitivity: perturb profile weights; classify stability.
 */
haeun_rb_result haeun_rb_run_sensitivity(haeun_rb_state *st);

/**
 * Workload robustness: document supported/unsupported variations.
 */
haeun_rb_result haeun_rb_workload_robustness(haeun_rb_state *st);

/**
 * Reconfiguration stress using R3.2 on a live VM (optional; may be dry).
 */
haeun_rb_result haeun_rb_reconfig_stress(haeun_rb_state *st, haeun_vm *vm,
                                         int dry_run);

/**
 * Verify R3.6-style recommendations against R3.7 evidence (non-destructive).
 */
haeun_rb_result haeun_rb_verify_recommendations(haeun_rb_state *st);

/**
 * Aggregate overall PASS/WARN/UNKNOWN/FAIL.
 */
void haeun_rb_aggregate(haeun_rb_state *st);

/**
 * Persist / report.
 */
haeun_rb_result haeun_rb_state_save(FILE *fp, const haeun_rb_state *st);
haeun_rb_result haeun_rb_state_load(FILE *fp, haeun_rb_state *st);
void haeun_rb_fprint(FILE *fp, const haeun_rb_state *st);
void haeun_rb_fprint_json(FILE *fp, const haeun_rb_state *st);

/**
 * Write reports under build/r37/ when out_dir provided.
 */
haeun_rb_result haeun_rb_export(const haeun_rb_state *st, const char *out_dir);

/**
 * CLI entry. Returns process exit code.
 */
int haeun_rb_cli_run(haeun_vm *vm, const char *build_dir, int json, int run,
                     int dry_run, int sensitivity, int stress, int verify,
                     const char *preset_name, unsigned reps,
                     const char *profile_name, const char *state_path,
                     const char *out_dir);

#endif
