#ifndef HAEUN_FEEDBACK_EXPERIMENT_H
#define HAEUN_FEEDBACK_EXPERIMENT_H

#include "haeun/eval_types.h"
#include "haeun/feedback_types.h"

#include <stdio.h>

enum { HAEUN_FB_EXP_SCHEMA = 1, HAEUN_FB_EXP_MODES = 3 };

typedef enum {
    HAEUN_FB_EXP_FIXED_RR = 0,
    HAEUN_FB_EXP_FIXED_FIFO = 1,
    HAEUN_FB_EXP_ADAPTIVE = 2
} haeun_fb_exp_mode;

typedef struct haeun_fb_exp_result {
    uint32_t schema_version;
    haeun_fb_exp_mode mode;
    char workload_id[HAEUN_EVAL_NAME_MAX];
    unsigned reps;
    unsigned success;
    unsigned fail;
    haeun_eval_opt mean_ready;
    haeun_eval_opt mean_jain;
    haeun_eval_opt mean_dispatch;
    unsigned transitions;
    char limitations[HAEUN_FB_MSG_LEN];
} haeun_fb_exp_result;

/**
 * Runs fixed RR or FIFO experiment via R2.2 runner.
 */
haeun_fb_result haeun_fb_exp_fixed(const char *build_dir,
                                   haeun_eval_wl_kind kind, uint32_t policy_id,
                                   unsigned reps, haeun_fb_exp_result *out);

/**
 * Runs Adaptive shadow/eval over same workload image (no claim of superiority).
 * execute=0 keeps SHADOW (no commits).
 */
haeun_fb_result haeun_fb_exp_adaptive(const char *build_dir,
                                      haeun_eval_wl_kind kind, unsigned reps,
                                      int execute, haeun_fb_exp_result *out);

/**
 * Prints one experiment result (text).
 */
void haeun_fb_exp_fprint(FILE *fp, const haeun_fb_exp_result *r);

/**
 * Prints one experiment result (JSON).
 */
void haeun_fb_exp_fprint_json(FILE *fp, const haeun_fb_exp_result *r);

#endif
