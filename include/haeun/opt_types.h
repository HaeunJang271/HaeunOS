#ifndef HAEUN_OPT_TYPES_H
#define HAEUN_OPT_TYPES_H

#include "haeun/eval_types.h"
#include "haeun/feedback_types.h"
#include "haeun/planner_types.h"

#include <stdint.h>

enum {
    HAEUN_OPT_SCHEMA = 1,
    HAEUN_OPT_ID_LEN = 48,
    HAEUN_OPT_MSG_LEN = 128,
    HAEUN_OPT_OBJ_MAX = 6,
    HAEUN_OPT_CAND_MAX = 4,
    HAEUN_OPT_WL_MAX = HAEUN_WL_COUNT,
    HAEUN_OPT_HIST_MAX = 16,
    HAEUN_OPT_DEFAULT_REPS = 3
};

typedef enum {
    HAEUN_OPT_OK = 0,
    HAEUN_OPT_BAD_ARGUMENT,
    HAEUN_OPT_IO,
    HAEUN_OPT_BAD_STATE,
    HAEUN_OPT_INCONCLUSIVE,
    HAEUN_OPT_REJECTED
} haeun_opt_result;

typedef enum {
    HAEUN_OPT_MODE_SHADOW = 0,
    HAEUN_OPT_MODE_OPTIMIZE,
    HAEUN_OPT_MODE_APPLY
} haeun_opt_mode;

typedef enum {
    HAEUN_OPT_PROF_BALANCED = 0,
    HAEUN_OPT_PROF_FAIRNESS,
    HAEUN_OPT_PROF_LATENCY,
    HAEUN_OPT_PROF_THROUGHPUT,
    HAEUN_OPT_PROF_LOW_OVERHEAD,
    HAEUN_OPT_PROF_COUNT
} haeun_opt_profile_id;

typedef enum {
    HAEUN_OPT_CAND_VALID = 0,
    HAEUN_OPT_CAND_INVALID,
    HAEUN_OPT_CAND_INSUFFICIENT,
    HAEUN_OPT_CAND_INCONCLUSIVE
} haeun_opt_cand_status;

typedef enum {
    HAEUN_OPT_DEC_SELECT = 0,
    HAEUN_OPT_DEC_NO_DECISION,
    HAEUN_OPT_DEC_EQUIVALENT,
    HAEUN_OPT_DEC_INCONCLUSIVE
} haeun_opt_decision;

typedef enum {
    HAEUN_OPT_DIR_MIN = 0,
    HAEUN_OPT_DIR_MAX
} haeun_opt_dir;

/** One objective in a profile (experimental weights). */
typedef struct haeun_opt_objective {
    char metric_name[40];
    haeun_opt_dir direction;
    double weight;
    double regress_limit; /* relative; 0 = any regress fails constraint */
    unsigned min_samples;
} haeun_opt_objective;

typedef struct haeun_opt_profile {
    haeun_opt_profile_id id;
    char name[24];
    unsigned obj_count;
    haeun_opt_objective objs[HAEUN_OPT_OBJ_MAX];
    double min_improve; /* relative score gap to prefer a winner */
    char rationale[HAEUN_OPT_MSG_LEN];
} haeun_opt_profile;

/** Aggregated metric stats for one candidate×workload. */
typedef struct haeun_opt_metric_stats {
    char name[40];
    unsigned n;
    unsigned missing;
    haeun_eval_opt mean;
    haeun_eval_opt median;
    haeun_eval_opt stddev;
    haeun_eval_opt min;
    haeun_eval_opt max;
    haeun_eval_opt ci95_lo; /* mean ± 1.96*s/√n when n≥2; else unavailable */
    haeun_eval_opt ci95_hi;
} haeun_opt_metric_stats;

typedef struct haeun_opt_candidate {
    uint32_t policy_id; /* 0=RR 1=FIFO */
    char name[16];
    haeun_opt_cand_status status;
    unsigned reps;
    unsigned success;
    unsigned fail;
    haeun_opt_metric_stats ready;
    haeun_opt_metric_stats jain;
    haeun_opt_metric_stats dispatch;
    haeun_opt_metric_stats completed;
    double score; /* weighted normalized; valid only if score_valid */
    int score_valid;
    int pareto_nondominated;
    char reason[HAEUN_OPT_MSG_LEN];
} haeun_opt_candidate;

typedef struct haeun_opt_wl_result {
    haeun_eval_wl_kind kind;
    char workload_id[HAEUN_EVAL_NAME_MAX];
    uint32_t fingerprint;
    unsigned cand_count;
    haeun_opt_candidate cands[HAEUN_OPT_CAND_MAX];
    haeun_opt_decision decision;
    uint32_t recommended_policy; /* valid if SELECT */
    char reason[HAEUN_OPT_MSG_LEN];
} haeun_opt_wl_result;

typedef struct haeun_opt_config {
    haeun_opt_profile_id profile_id;
    unsigned reps;
    unsigned min_success;
    double min_improve;
    uint32_t cooldown_cycles;
    int experimental;
    char build_dir[HAEUN_EVAL_PATH_MAX];
    char software_version[24];
    uint32_t config_fingerprint;
} haeun_opt_config;

typedef struct haeun_opt_recommendation {
    int valid;
    uint32_t schema_version;
    char experiment_id[HAEUN_OPT_ID_LEN];
    haeun_opt_profile_id profile_id;
    haeun_eval_wl_kind workload;
    uint32_t recommended_policy;
    haeun_opt_decision decision;
    uint32_t generation;
    uint32_t config_fingerprint;
    char reason[HAEUN_OPT_MSG_LEN];
    char limitations[HAEUN_OPT_MSG_LEN];
} haeun_opt_recommendation;

typedef struct haeun_opt_state {
    uint32_t schema_version;
    char controller_version[24];
    haeun_opt_mode mode;
    haeun_opt_config cfg;
    haeun_opt_profile profile;
    unsigned wl_count;
    haeun_opt_wl_result workloads[HAEUN_OPT_WL_MAX];
    haeun_opt_recommendation last_rec;
    uint32_t cycle_id;
    uint32_t cooldown_remaining;
    unsigned hist_count;
    haeun_opt_recommendation history[HAEUN_OPT_HIST_MAX];
} haeun_opt_state;

#endif
