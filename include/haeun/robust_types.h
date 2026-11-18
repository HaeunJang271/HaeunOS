#ifndef HAEUN_ROBUST_TYPES_H
#define HAEUN_ROBUST_TYPES_H

#include "haeun/opt_types.h"

#include <stdint.h>

enum {
    HAEUN_RB_SCHEMA = 1,
    HAEUN_RB_ID_LEN = 64,
    HAEUN_RB_MSG_LEN = 160,
    HAEUN_RB_REC_MAX = 256,
    HAEUN_RB_WL_MAX = HAEUN_WL_COUNT
};

typedef enum {
    HAEUN_RB_OK = 0,
    HAEUN_RB_BAD_ARGUMENT,
    HAEUN_RB_IO,
    HAEUN_RB_BAD_STATE,
    HAEUN_RB_FAIL
} haeun_rb_result;

typedef enum {
    HAEUN_RB_PRESET_SMOKE = 0,     /* 3 */
    HAEUN_RB_PRESET_STANDARD,      /* 10 */
    HAEUN_RB_PRESET_EXTENDED,      /* 30 */
    HAEUN_RB_PRESET_RESEARCH,      /* 100 */
    HAEUN_RB_PRESET_COUNT
} haeun_rb_preset;

typedef enum {
    HAEUN_RB_EXACT_MATCH = 0,
    HAEUN_RB_METRIC_MATCH,
    HAEUN_RB_DIVERGENT,
    HAEUN_RB_INCOMPLETE,
    HAEUN_RB_INVALID
} haeun_rb_replay_class;

typedef enum {
    HAEUN_RB_REL_SUFFICIENT = 0,
    HAEUN_RB_REL_INSUFFICIENT,
    HAEUN_RB_REL_INCONCLUSIVE,
    HAEUN_RB_REL_INVALID
} haeun_rb_reliability;

typedef enum {
    HAEUN_RB_STABLE = 0,
    HAEUN_RB_SENSITIVE,
    HAEUN_RB_UNSTABLE,
    HAEUN_RB_SENS_INSUFFICIENT
} haeun_rb_sensitivity;

typedef enum {
    HAEUN_RB_VERIFY_CONFIRMED = 0,
    HAEUN_RB_VERIFY_CONDITIONAL,
    HAEUN_RB_VERIFY_NOT_CONFIRMED,
    HAEUN_RB_VERIFY_INSUFFICIENT,
    HAEUN_RB_VERIFY_INVALID
} haeun_rb_verify_status;

typedef enum {
    HAEUN_RB_OVERALL_PASS = 0,
    HAEUN_RB_OVERALL_WARN,
    HAEUN_RB_OVERALL_UNKNOWN,
    HAEUN_RB_OVERALL_FAIL
} haeun_rb_overall;

typedef struct haeun_rb_config {
    haeun_rb_preset preset;
    unsigned reps;
    unsigned min_success;
    char build_dir[HAEUN_EVAL_PATH_MAX];
    char build_id[40];
    char source_revision[40];
    uint32_t config_hash;
    haeun_opt_profile_id profile_id;
} haeun_rb_config;

/** One isolated candidate run fingerprint (metric-level). */
typedef struct haeun_rb_run_fp {
    int ok;
    uint32_t exit_code;
    uint64_t total_dispatch;
    uint64_t context_switches;
    uint64_t timer_irqs;
    uint64_t total_run;
    uint64_t total_ready;
    unsigned completed;
    char fail_reason[HAEUN_RB_MSG_LEN];
} haeun_rb_run_fp;

typedef struct haeun_rb_experiment {
    char experiment_id[HAEUN_RB_ID_LEN];
    uint32_t config_hash;
    uint32_t workload_seed;
    haeun_eval_wl_kind workload;
    uint32_t policy_id;
    unsigned repetition;
    int ok;
    haeun_rb_run_fp fp;
} haeun_rb_experiment;

typedef struct haeun_rb_wl_report {
    haeun_eval_wl_kind kind;
    char workload_id[HAEUN_EVAL_NAME_MAX];
    uint32_t seed;
    haeun_opt_wl_result opt; /* RR/FIFO candidates + decision */
    haeun_rb_replay_class replay;
    haeun_rb_reliability reliability;
    haeun_rb_sensitivity sensitivity;
    haeun_rb_verify_status verify;
    haeun_opt_decision original_decision;
    uint32_t original_policy;
    uint32_t verified_policy;
    unsigned reconfig_ok;
    unsigned reconfig_fail;
    char reason[HAEUN_RB_MSG_LEN];
} haeun_rb_wl_report;

typedef struct haeun_rb_state {
    uint32_t schema_version;
    char controller_version[24];
    haeun_rb_config cfg;
    unsigned wl_count;
    haeun_rb_wl_report workloads[HAEUN_RB_WL_MAX];
    unsigned exp_count;
    haeun_rb_experiment experiments[HAEUN_RB_REC_MAX];
    unsigned stress_transitions;
    unsigned stress_failures;
    haeun_rb_overall overall;
    char overall_reason[HAEUN_RB_MSG_LEN];
    char limitations[HAEUN_RB_MSG_LEN];
} haeun_rb_state;

#endif
