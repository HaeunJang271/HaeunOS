#ifndef HAEUN_PLANNER_TYPES_H
#define HAEUN_PLANNER_TYPES_H

#include "haeun/eval_types.h"
#include "haeun/pval_report.h"
#include "haeun/telem_inspect.h"

#include <stdint.h>

enum {
    HAEUN_PLAN_SCHEMA = 1,
    HAEUN_PLAN_ID_LEN = 40,
    HAEUN_PLAN_MSG_LEN = 128,
    HAEUN_PLAN_EVIDENCE_MAX = 12,
    HAEUN_PLAN_HIST_MAX = 8
};

typedef enum {
    HAEUN_PLAN_OK = 0,
    HAEUN_PLAN_BAD_ARGUMENT,
    HAEUN_PLAN_NO_TELEM,
    HAEUN_PLAN_STALE,
    HAEUN_PLAN_REJECTED
} haeun_plan_result;

typedef enum {
    HAEUN_PLAN_KEEP_CURRENT = 0,
    HAEUN_PLAN_RECOMMEND_CHANGE,
    HAEUN_PLAN_NO_DECISION,
    HAEUN_PLAN_REJECTED_BY_SAFETY
} haeun_plan_decision;

typedef enum {
    HAEUN_PLAN_CONF_NONE = 0,
    HAEUN_PLAN_CONF_LOW,
    HAEUN_PLAN_CONF_MEDIUM,
    HAEUN_PLAN_CONF_HIGH
} haeun_plan_confidence;

typedef enum {
    HAEUN_PLAN_EV_OK = 0,
    HAEUN_PLAN_EV_ZERO,
    HAEUN_PLAN_EV_MISSING,
    HAEUN_PLAN_EV_INVALID,
    HAEUN_PLAN_EV_UNKNOWN,
    HAEUN_PLAN_EV_NA
} haeun_plan_ev_kind;

typedef enum {
    HAEUN_PLAN_WL_UNKNOWN = 0,
    HAEUN_PLAN_WL_CPU_BOUND,
    HAEUN_PLAN_WL_SHORT_JOB,
    HAEUN_PLAN_WL_MIXED,
    HAEUN_PLAN_WL_BURSTY,
    HAEUN_PLAN_WL_BLOCK_SLEEP,
    HAEUN_PLAN_WL_BLOCK_WAIT,
    HAEUN_PLAN_WL_IDLE
} haeun_plan_wl_class;

/** Named evidence sample with explicit availability. */
typedef struct haeun_plan_evidence {
    char name[48];
    haeun_plan_ev_kind kind;
    double value;
    char note[HAEUN_PLAN_MSG_LEN];
} haeun_plan_evidence;

/** Configurable experimental thresholds (documented; not proven optima). */
typedef struct haeun_plan_config {
    double ready_run_ratio_high;   /* experimental: suggest FIFO interest */
    double sleep_share_high;       /* experimental: blocking/sleep share */
    double dispatch_run_high;      /* experimental: short-job signal */
    uint32_t min_dwell_ticks;      /* stability: min ticks between changes */
    uint32_t cooldown_ticks;       /* stability: post-decision cooldown */
    unsigned consecutive_needed;   /* identical recommend streak */
    double hysteresis_delta;       /* ignore ratio noise below this */
    uint32_t plan_ttl_ticks;       /* plan expiry in guest timer ticks */
    int experimental_rules;        /* 1 = allow recommend_change rules */
} haeun_plan_config;

/** Immutable planner observation (copied scalars; no guest pointers). */
typedef struct haeun_plan_obs {
    int telem_ok;
    int policy_ok;
    uint32_t policy_id;
    uint64_t timer_count;
    uint32_t snapshot_generation;
    haeun_telem_report telem;
    haeun_eval_run_metrics metrics;
    int metrics_ok;
} haeun_plan_obs;

/** Policy plan object (serializable). */
typedef struct haeun_plan {
    char plan_id[HAEUN_PLAN_ID_LEN];
    uint64_t created_at_ticks;
    uint64_t expires_at_ticks;
    uint32_t source_policy;
    uint32_t target_policy;
    haeun_plan_decision decision;
    haeun_plan_wl_class workload_class;
    haeun_plan_confidence confidence;
    uint32_t snapshot_generation;
    char reason[HAEUN_PLAN_MSG_LEN];
    char rejection_reason[HAEUN_PLAN_MSG_LEN];
    char stability_status[HAEUN_PLAN_MSG_LEN];
    char safety_status[HAEUN_PLAN_MSG_LEN];
    unsigned evidence_count;
    haeun_plan_evidence evidence[HAEUN_PLAN_EVIDENCE_MAX];
    int shadow_mode;
    int execute_requested;
    int safety_ran;
    haeun_pval_status safety_overall;
    int safety_preconditions_ok;
    int txn_attempted;
    int txn_committed;
    char txn_id[HAEUN_PLAN_ID_LEN];
} haeun_plan;

/** Host-side planner session state for stability (not guest RAM). */
typedef struct haeun_plan_session {
    haeun_plan_config cfg;
    uint32_t last_policy;
    uint64_t last_decision_ticks;
    uint32_t last_recommend_target;
    unsigned consecutive_same;
    unsigned hist_count;
    haeun_plan_decision hist_dec[HAEUN_PLAN_HIST_MAX];
    uint32_t hist_tgt[HAEUN_PLAN_HIST_MAX];
    uint32_t plan_seq;
} haeun_plan_session;

/**
 * Fills conservative experimental defaults.
 */
void haeun_plan_config_default(haeun_plan_config *cfg);

/**
 * Initializes session with defaults.
 */
void haeun_plan_session_init(haeun_plan_session *sess);

#endif
