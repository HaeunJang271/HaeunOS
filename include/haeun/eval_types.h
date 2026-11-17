#ifndef HAEUN_EVAL_TYPES_H
#define HAEUN_EVAL_TYPES_H

#include "haeun/telem_inspect.h"

#include <stddef.h>
#include <stdint.h>

enum {
    HAEUN_EVAL_SCHEMA = 1,
    HAEUN_EVAL_NAME_MAX = 48,
    HAEUN_EVAL_PATH_MAX = 256,
    HAEUN_EVAL_REASON_MAX = 96,
    HAEUN_EVAL_MAX_RUNS = 16,
    HAEUN_EVAL_DEFAULT_REPS = 3,
    HAEUN_EVAL_AS_SLOTS = 3
};

/** Build label recorded in reports (not a formal release version). */
#define HAEUN_EVAL_BUILD_ID "haeun-os-1.0+aros-r30"

enum {
    HAEUN_EVAL_POLICY_RR = 0,
    HAEUN_EVAL_POLICY_FCFS = 1
};

typedef enum {
    HAEUN_EVAL_OK = 0,
    HAEUN_EVAL_BAD_ARGUMENT,
    HAEUN_EVAL_IO,
    HAEUN_EVAL_NO_WORKLOAD,
    HAEUN_EVAL_RUN_FAILED,
    HAEUN_EVAL_INCOMPARABLE
} haeun_eval_result;

typedef enum {
    HAEUN_WL_CPU_BOUND = 0,
    HAEUN_WL_SHORT_JOB,
    HAEUN_WL_MIXED,
    HAEUN_WL_BURSTY,
    HAEUN_WL_BLOCK_SLEEP,
    HAEUN_WL_BLOCK_WAIT,
    HAEUN_WL_COUNT
} haeun_eval_wl_kind;

/** Optional floating metric; unavailable values are never coerced to 0. */
typedef struct haeun_eval_opt {
    int available;
    double value;
    char reason[HAEUN_EVAL_REASON_MAX];
} haeun_eval_opt;

/** Workload catalog entry (host-side; maps to a guest HXF image). */
typedef struct haeun_eval_workload {
    haeun_eval_wl_kind kind;
    char name[HAEUN_EVAL_NAME_MAX];
    char category[HAEUN_EVAL_NAME_MAX];
    char image_name[HAEUN_EVAL_NAME_MAX];
    uint32_t expected_exit;
    uint32_t expected_min_pids;
    uint64_t step_cap;
    uint32_t seed;
    int uses_rng;
} haeun_eval_workload;

/** Per-run raw aggregates derived from telemetry (participating slots only). */
typedef struct haeun_eval_run_metrics {
    int ok;
    char fail_reason[HAEUN_EVAL_REASON_MAX];
    uint32_t exit_code;
    uint64_t steps;
    uint32_t participants;
    uint32_t completed;
    uint32_t incomplete;
    uint64_t total_run_ticks;
    uint64_t total_ready_wait;
    uint64_t total_sleep;
    uint64_t total_blocked;
    uint64_t total_dispatch;
    uint32_t context_switches;
    uint32_t timer_irqs;
    uint32_t slot_run[HAEUN_TELEM_NPROC];
    uint32_t slot_ready[HAEUN_TELEM_NPROC];
    uint32_t slot_complete_valid[HAEUN_TELEM_NPROC];
    uint32_t slot_complete_lo[HAEUN_TELEM_NPROC];
    uint32_t slot_pid[HAEUN_TELEM_NPROC];
    int slot_included[HAEUN_TELEM_NPROC];
    haeun_eval_opt avg_ready_wait;
    haeun_eval_opt ctx_dispatch_ratio;
} haeun_eval_run_metrics;

/** One repetition result. */
typedef struct haeun_eval_run {
    unsigned index;
    haeun_eval_run_metrics metrics;
} haeun_eval_run;

/** Full experiment (workload + repetitions). */
typedef struct haeun_eval_experiment {
    char experiment_id[HAEUN_EVAL_NAME_MAX];
    char build_id[HAEUN_EVAL_NAME_MAX];
    char policy_name[HAEUN_EVAL_NAME_MAX];
    uint32_t policy_id;
    haeun_eval_workload workload;
    unsigned requested_reps;
    unsigned success_count;
    unsigned fail_count;
    haeun_eval_run runs[HAEUN_EVAL_MAX_RUNS];
} haeun_eval_experiment;

#endif
