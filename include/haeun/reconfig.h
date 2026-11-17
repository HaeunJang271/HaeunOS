#ifndef HAEUN_RECONFIG_H
#define HAEUN_RECONFIG_H

#include "haeun/pval_report.h"
#include "haeun/vm.h"

#include <stdio.h>
#include <stdint.h>

enum {
    HAEUN_RECFG_LOG_MAX = 32,
    HAEUN_RECFG_MSG_LEN = 128,
    HAEUN_RECFG_ID_LEN = 40
};

typedef enum {
    HAEUN_RECFG_OK = 0,
    HAEUN_RECFG_BAD_ARGUMENT,
    HAEUN_RECFG_BUSY,
    HAEUN_RECFG_REJECTED,
    HAEUN_RECFG_FAILED,
    HAEUN_RECFG_BAD_STATE
} haeun_recfg_result;

typedef enum {
    HAEUN_RECFG_ST_IDLE = 0,
    HAEUN_RECFG_ST_REQUESTED,
    HAEUN_RECFG_ST_PREFLIGHT,
    HAEUN_RECFG_ST_SNAPSHOT,
    HAEUN_RECFG_ST_VALIDATING,
    HAEUN_RECFG_ST_PREPARED,
    HAEUN_RECFG_ST_COMMITTING,
    HAEUN_RECFG_ST_COMMITTED,
    HAEUN_RECFG_ST_VERIFYING,
    HAEUN_RECFG_ST_ROLLING_BACK,
    HAEUN_RECFG_ST_ROLLED_BACK,
    HAEUN_RECFG_ST_REJECTED,
    HAEUN_RECFG_ST_FAILED
} haeun_recfg_state;

typedef enum {
    HAEUN_RECFG_FI_NONE = 0,
    HAEUN_RECFG_FI_BEFORE_SNAPSHOT,
    HAEUN_RECFG_FI_AFTER_SNAPSHOT,
    HAEUN_RECFG_FI_AFTER_VALIDATE,
    HAEUN_RECFG_FI_BEFORE_PREPARE,
    HAEUN_RECFG_FI_AFTER_PREPARE,
    HAEUN_RECFG_FI_BEFORE_COMMIT,
    HAEUN_RECFG_FI_AFTER_WRITE,
    HAEUN_RECFG_FI_AFTER_VERIFY,
    HAEUN_RECFG_FI_DURING_ROLLBACK,
    HAEUN_RECFG_FI_AFTER_ROLLBACK
} haeun_recfg_fi_point;

typedef struct haeun_recfg_log_entry {
    haeun_recfg_state state;
    char message[HAEUN_RECFG_MSG_LEN];
} haeun_recfg_log_entry;

/** Host-owned transaction; no live guest RAM pointers retained. */
typedef struct haeun_recfg_txn {
    char txn_id[HAEUN_RECFG_ID_LEN];
    haeun_recfg_state state;
    uint32_t source_policy;
    uint32_t target_policy;
    uint32_t observed_policy_at_request;
    uint32_t snapshot_generation;
    uint32_t sealed_policy;
    uint32_t sealed_curr;
    uint64_t sealed_timer_count;
    uint32_t seal[HAEUN_PVAL_SEAL_WORDS];
    int seal_valid;
    haeun_pval_report validation;
    int validation_ran;
    int policy_id_restored;
    char reject_reason[HAEUN_RECFG_MSG_LEN];
    unsigned log_count;
    haeun_recfg_log_entry log[HAEUN_RECFG_LOG_MAX];
} haeun_recfg_txn;

/**
 * Clears failure-injection point (test-only; default NONE).
 */
void haeun_recfg_fi_clear(void);

/**
 * Arms a test-only failure injection point. No effect when NONE.
 */
void haeun_recfg_fi_arm(haeun_recfg_fi_point point);

/**
 * Test helper: returns 1 once when the armed point matches, then clears.
 */
int haeun_recfg_fi_hit(haeun_recfg_fi_point point);

/**
 * Begins a transaction expecting source_policy, targeting target_policy.
 */
haeun_recfg_result haeun_recfg_request(haeun_recfg_txn *txn, haeun_vm *vm,
                                       uint32_t source_policy,
                                       uint32_t target_policy);

/**
 * Captures snapshot and runs R3.1 validation. Does not change policy.
 */
haeun_recfg_result haeun_recfg_prepare(haeun_recfg_txn *txn, haeun_vm *vm);

/**
 * Serialized commit of the policy word at a between-step safe point.
 */
haeun_recfg_result haeun_recfg_commit(haeun_recfg_txn *txn, haeun_vm *vm);

/**
 * Restores previous policy ID if a commit write occurred or was requested.
 */
haeun_recfg_result haeun_recfg_rollback(haeun_recfg_txn *txn, haeun_vm *vm);

/**
 * Returns the current transaction state.
 */
haeun_recfg_state haeun_recfg_status(const haeun_recfg_txn *txn);

/**
 * Prints human-readable transaction report.
 */
void haeun_recfg_report_fprint(FILE *fp, const haeun_recfg_txn *txn);

/**
 * Prints JSON transaction report.
 */
void haeun_recfg_report_fprint_json(FILE *fp, const haeun_recfg_txn *txn);

#endif
