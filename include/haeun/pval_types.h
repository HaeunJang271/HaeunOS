#ifndef HAEUN_PVAL_TYPES_H
#define HAEUN_PVAL_TYPES_H

#include <stdint.h>

enum {
    HAEUN_PVAL_SCHEMA = 1,
    HAEUN_PVAL_NPROC = 4,
    HAEUN_PVAL_AS_SLOTS = 3,
    HAEUN_PVAL_CHECK_MAX = 48,
    HAEUN_PVAL_ID_LEN = 48,
    HAEUN_PVAL_MSG_LEN = 128,
    HAEUN_PVAL_POLICY_RR = 0,
    HAEUN_PVAL_POLICY_FCFS = 1,
    HAEUN_PVAL_SEAL_WORDS = 8
};

typedef enum {
    HAEUN_PVAL_OK = 0,
    HAEUN_PVAL_BAD_ARGUMENT,
    HAEUN_PVAL_NO_PCB,
    HAEUN_PVAL_MUTATION
} haeun_pval_result;

typedef enum {
    HAEUN_PVAL_PASS = 0,
    HAEUN_PVAL_FAIL,
    HAEUN_PVAL_WARN,
    HAEUN_PVAL_UNKNOWN,
    HAEUN_PVAL_NOT_APPLICABLE
} haeun_pval_status;

typedef enum {
    HAEUN_PVAL_FIELD_VALID = 0,
    HAEUN_PVAL_FIELD_INVALID,
    HAEUN_PVAL_FIELD_UNAVAILABLE,
    HAEUN_PVAL_FIELD_STALE,
    HAEUN_PVAL_FIELD_NA
} haeun_pval_field_kind;

/** One named check result. */
typedef struct haeun_pval_check {
    char id[HAEUN_PVAL_ID_LEN];
    haeun_pval_status status;
    char explanation[HAEUN_PVAL_MSG_LEN];
} haeun_pval_check;

/** Per-slot view for validation (copied scalars only). */
typedef struct haeun_pval_slot {
    uint32_t slot;
    uint32_t state;
    uint32_t pid;
    uint32_t ppid;
    uint32_t epc;
    uint32_t ptbr;
    uint32_t ready_since;
    uint32_t ready_seq_lo;
    uint32_t ready_seq_hi;
    uint32_t run_ticks;
    uint32_t ready_wait_ticks;
    uint32_t sleep_ticks;
    uint32_t blocked_ticks;
    uint32_t dispatch_count;
    uint32_t complete_lo;
    uint32_t complete_valid;
    int as_slot_valid;
    int is_participant;
    haeun_pval_field_kind telem_kind;
} haeun_pval_slot;

/** Immutable copied snapshot; no live guest pointers. */
typedef struct haeun_pval_snapshot {
    uint32_t policy_id;
    uint32_t curr_index;
    uint32_t idle_flag;
    uint32_t timer_irqs;
    uint32_t context_switches;
    uint64_t timer_count;
    uint64_t timer_compare;
    uint32_t timer_control;
    uint32_t csr_ptbr;
    uint32_t host_pc;
    uint64_t host_steps_marker;
    haeun_pval_slot slots[HAEUN_PVAL_NPROC];
    unsigned participant_count;
    unsigned ready_eligible_count;
    uint32_t ready_seq_global_lo;
    uint32_t ready_seq_global_hi;
    uint32_t ready_seq_wrap;
    /** Seal of PCB page + policy + timer CSRs for read-only audit. */
    uint32_t seal[HAEUN_PVAL_SEAL_WORDS];
    int seal_valid;
} haeun_pval_snapshot;

#endif
