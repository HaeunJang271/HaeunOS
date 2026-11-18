#ifndef HAEUN_FEEDBACK_TYPES_H
#define HAEUN_FEEDBACK_TYPES_H

#include "haeun/eval_types.h"
#include "haeun/planner_types.h"

#include <stdint.h>

enum {
    HAEUN_FB_SCHEMA = 1,
    HAEUN_FB_ID_LEN = 48,
    HAEUN_FB_MSG_LEN = 128,
    HAEUN_FB_HIST_MAX = 16,
    HAEUN_FB_OBJ_MAX = 6
};

typedef enum {
    HAEUN_FB_OK = 0,
    HAEUN_FB_BAD_ARGUMENT,
    HAEUN_FB_INCOMPARABLE,
    HAEUN_FB_IO,
    HAEUN_FB_BAD_STATE,
    HAEUN_FB_REJECTED
} haeun_fb_result;

typedef enum {
    HAEUN_FB_VALID = 0,
    HAEUN_FB_UNAVAILABLE,
    HAEUN_FB_INVALID,
    HAEUN_FB_INSUFFICIENT_SAMPLE
} haeun_fb_validity;

typedef enum {
    HAEUN_FB_IMPROVED = 0,
    HAEUN_FB_DEGRADED,
    HAEUN_FB_NEUTRAL,
    HAEUN_FB_INCONCLUSIVE
} haeun_fb_outcome;

typedef enum {
    HAEUN_FB_MODE_SHADOW = 0,
    HAEUN_FB_MODE_ADAPTIVE
} haeun_fb_mode;

typedef enum {
    HAEUN_FB_DIR_LOWER_BETTER = 0,
    HAEUN_FB_DIR_HIGHER_BETTER
} haeun_fb_direction;

/** Single named metric with explicit validity (never coerce missing→0). */
typedef struct haeun_fb_metric {
    char name[40];
    haeun_fb_validity validity;
    double value;
    char note[HAEUN_FB_MSG_LEN];
} haeun_fb_metric;

/** Bundle derived from one evaluation window. */
typedef struct haeun_fb_sample {
    int ok;
    uint32_t policy_id;
    uint64_t timer_count;
    uint32_t participants;
    char workload_id[HAEUN_EVAL_NAME_MAX];
    uint32_t fingerprint; /* stable hash of workload+params */
    haeun_fb_metric total_run;
    haeun_fb_metric avg_ready_wait;
    haeun_fb_metric max_ready_wait;
    haeun_fb_metric jain_fairness;
    haeun_fb_metric dispatch_count;
    haeun_fb_metric ctx_switches;
    haeun_fb_metric timer_irqs;
    haeun_fb_metric completed;
    haeun_fb_metric completion_latency;
} haeun_fb_sample;

/** One multi-objective criterion (experimental defaults; not proven optima). */
typedef struct haeun_fb_objective {
    char metric_name[40];
    haeun_fb_direction direction;
    double weight;
    double improve_threshold; /* relative delta to count as improve */
    double regress_threshold; /* relative delta to count as regress */
    unsigned min_participants;
} haeun_fb_objective;

typedef struct haeun_fb_config {
    unsigned obj_count;
    haeun_fb_objective objs[HAEUN_FB_OBJ_MAX];
    unsigned min_valid_objectives;
    unsigned consecutive_improve_needed;
    unsigned consecutive_regress_needed;
    uint32_t cooldown_cycles;
    int experimental;
} haeun_fb_config;

typedef struct haeun_fb_obj_result {
    char metric_name[40];
    haeun_fb_validity validity;
    double before;
    double after;
    double relative_delta;
    int improved;
    int degraded;
    char note[HAEUN_FB_MSG_LEN];
} haeun_fb_obj_result;

/** Structured assessment of a before→after policy effect. */
typedef struct haeun_fb_assessment {
    haeun_fb_outcome outcome;
    int comparable;
    unsigned obj_count;
    haeun_fb_obj_result objs[HAEUN_FB_OBJ_MAX];
    double weighted_score; /* +improve −regress; only when comparable */
    int score_valid;
    char reason[HAEUN_FB_MSG_LEN];
    char limitations[HAEUN_FB_MSG_LEN];
} haeun_fb_assessment;

typedef struct haeun_fb_hist_entry {
    uint32_t cycle_id;
    uint32_t from_policy;
    uint32_t to_policy;
    haeun_plan_decision decision;
    haeun_fb_outcome outcome;
    int txn_committed;
    char reason[HAEUN_FB_MSG_LEN];
} haeun_fb_hist_entry;

/** Host-side adaptive controller state (not guest RAM). */
typedef struct haeun_fb_state {
    uint32_t schema_version;
    char controller_version[24];
    uint32_t cycle_id;
    haeun_fb_mode mode;
    uint32_t current_policy;
    uint32_t previous_policy;
    haeun_plan_decision previous_decision;
    haeun_fb_outcome previous_outcome;
    int previous_outcome_valid;
    haeun_fb_sample baseline;
    haeun_fb_sample post;
    uint32_t workload_fingerprint;
    uint32_t measurement_generation;
    haeun_plan_confidence confidence;
    uint32_t cooldown_remaining;
    unsigned consecutive_improvements;
    unsigned consecutive_regressions;
    unsigned hist_count;
    haeun_fb_hist_entry history[HAEUN_FB_HIST_MAX];
    haeun_fb_config cfg;
} haeun_fb_state;

#endif
