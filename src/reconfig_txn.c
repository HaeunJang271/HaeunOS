#include "haeun/reconfig.h"

#include "haeun/pval_snapshot.h"
#include "haeun/pval_validate.h"

#include <stdio.h>
#include <string.h>

enum { POL_OFF = 0x0000A474u };

static uint32_t g_txn_seq;
static int g_busy;

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static int ram_ok(const haeun_vm *vm) {
    return vm != NULL && vm->ram != NULL && vm->ram_size > (size_t)(POL_OFF + 4u);
}

static void log_msg(haeun_recfg_txn *txn, const char *msg) {
    haeun_recfg_log_entry *e;
    if (txn->log_count >= HAEUN_RECFG_LOG_MAX)
        return;
    e = &txn->log[txn->log_count++];
    e->state = txn->state;
    snprintf(e->message, sizeof(e->message), "%s", msg);
}

static void reject(haeun_recfg_txn *txn, const char *why) {
    snprintf(txn->reject_reason, sizeof(txn->reject_reason), "%s", why);
    txn->state = HAEUN_RECFG_ST_REJECTED;
    log_msg(txn, why);
    g_busy = 0;
}

static int known(uint32_t id) {
    return id == HAEUN_PVAL_POLICY_RR || id == HAEUN_PVAL_POLICY_FCFS;
}

/** WARN ids that do not block prepare (documented non-blocking). */
static int warn_nonblocking(const char *id) {
    return strcmp(id, "TELEM.stale_free") == 0 ||
           strcmp(id, "CUR.running_match") == 0;
}

static int invariants_ok_for_prepare(const haeun_pval_invariant_report *inv) {
    unsigned i;
    if (inv->overall == HAEUN_PVAL_FAIL || inv->overall == HAEUN_PVAL_UNKNOWN)
        return 0;
    if (inv->overall == HAEUN_PVAL_PASS ||
        inv->overall == HAEUN_PVAL_NOT_APPLICABLE)
        return 1;
    /* WARN: only allow if every WARN is non-blocking. */
    for (i = 0; i < inv->count; ++i) {
        if (inv->checks[i].status == HAEUN_PVAL_WARN &&
            !warn_nonblocking(inv->checks[i].id))
            return 0;
        if (inv->checks[i].status == HAEUN_PVAL_FAIL ||
            inv->checks[i].status == HAEUN_PVAL_UNKNOWN)
            return 0;
    }
    return 1;
}

/**
 * Begins a transaction expecting source_policy, targeting target_policy.
 */
haeun_recfg_result haeun_recfg_request(haeun_recfg_txn *txn, haeun_vm *vm,
                                       uint32_t source_policy,
                                       uint32_t target_policy) {
    uint32_t cur;
    if (txn == NULL || !ram_ok(vm))
        return HAEUN_RECFG_BAD_ARGUMENT;
    if (g_busy)
        return HAEUN_RECFG_BUSY;
    memset(txn, 0, sizeof(*txn));
    g_txn_seq++;
    snprintf(txn->txn_id, sizeof(txn->txn_id), "r32-%u", g_txn_seq);
    txn->source_policy = source_policy;
    txn->target_policy = target_policy;
    txn->state = HAEUN_RECFG_ST_REQUESTED;
    g_busy = 1;
    log_msg(txn, "request accepted");
    if (!known(source_policy) || !known(target_policy)) {
        reject(txn, "unknown source or target policy");
        return HAEUN_RECFG_REJECTED;
    }
    cur = rd32(vm->ram + POL_OFF);
    txn->observed_policy_at_request = cur;
    if (cur != source_policy) {
        reject(txn, "active policy does not match expected source");
        return HAEUN_RECFG_REJECTED;
    }
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_BEFORE_SNAPSHOT)) {
        reject(txn, "FI before snapshot");
        return HAEUN_RECFG_REJECTED;
    }
    return HAEUN_RECFG_OK;
}

/**
 * Captures snapshot and runs R3.1 validation. Does not change policy.
 */
haeun_recfg_result haeun_recfg_prepare(haeun_recfg_txn *txn, haeun_vm *vm) {
    haeun_pval_result pr;
    if (txn == NULL || !ram_ok(vm))
        return HAEUN_RECFG_BAD_ARGUMENT;
    if (txn->state != HAEUN_RECFG_ST_REQUESTED)
        return HAEUN_RECFG_BAD_STATE;
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_BEFORE_PREPARE)) {
        reject(txn, "FI before prepare");
        return HAEUN_RECFG_REJECTED;
    }
    txn->state = HAEUN_RECFG_ST_PREFLIGHT;
    log_msg(txn, "preflight");
    txn->state = HAEUN_RECFG_ST_SNAPSHOT;
    txn->state = HAEUN_RECFG_ST_VALIDATING;
    pr = haeun_pval_validate(vm, txn->target_policy, &txn->validation);
    txn->validation_ran = 1;
    if (pr != HAEUN_PVAL_OK) {
        reject(txn, "validation call failed");
        return HAEUN_RECFG_REJECTED;
    }
    txn->seal_valid = txn->validation.snap.seal_valid;
    memcpy(txn->seal, txn->validation.snap.seal, sizeof(txn->seal));
    txn->sealed_policy = txn->validation.snap.policy_id;
    txn->sealed_curr = txn->validation.snap.curr_index;
    txn->sealed_timer_count = txn->validation.snap.timer_count;
    txn->snapshot_generation = g_txn_seq;
    log_msg(txn, "snapshot+validation complete");
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_AFTER_SNAPSHOT)) {
        reject(txn, "FI after snapshot");
        return HAEUN_RECFG_REJECTED;
    }
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_AFTER_VALIDATE)) {
        reject(txn, "FI after validate");
        return HAEUN_RECFG_REJECTED;
    }
    if (txn->validation.read_only != HAEUN_PVAL_PASS) {
        reject(txn, "read-only audit failed");
        return HAEUN_RECFG_REJECTED;
    }
    if (!invariants_ok_for_prepare(&txn->validation.invariants)) {
        reject(txn, "mandatory invariants not PASS (UNKNOWN/FAIL/blocking WARN)");
        return HAEUN_RECFG_REJECTED;
    }
    if (!txn->validation.compat_ran ||
        !txn->validation.compat.transition_preconditions_satisfied) {
        reject(txn, "compatibility preconditions not satisfied (UNKNOWN≠PASS)");
        return HAEUN_RECFG_REJECTED;
    }
    if (txn->validation.snap.policy_id != txn->source_policy) {
        reject(txn, "snapshot policy mismatch vs source");
        return HAEUN_RECFG_REJECTED;
    }
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_AFTER_PREPARE)) {
        reject(txn, "FI after prepare");
        return HAEUN_RECFG_REJECTED;
    }
    txn->state = HAEUN_RECFG_ST_PREPARED;
    log_msg(txn, "prepared");
    return HAEUN_RECFG_OK;
}

/**
 * Serialized commit of the policy word at a between-step safe point.
 */
haeun_recfg_result haeun_recfg_commit(haeun_recfg_txn *txn, haeun_vm *vm) {
    uint32_t cur;
    if (txn == NULL || !ram_ok(vm))
        return HAEUN_RECFG_BAD_ARGUMENT;
    if (txn->state != HAEUN_RECFG_ST_PREPARED)
        return HAEUN_RECFG_BAD_STATE;
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_BEFORE_COMMIT)) {
        reject(txn, "FI before commit");
        return HAEUN_RECFG_REJECTED;
    }
    txn->state = HAEUN_RECFG_ST_COMMITTING;
    log_msg(txn, "committing at between-step safe point");
    if (haeun_pval_snapshot_verify_unchanged(vm, &txn->validation.snap) !=
        HAEUN_PVAL_OK) {
        reject(txn, "stale snapshot before commit");
        return HAEUN_RECFG_REJECTED;
    }
    cur = rd32(vm->ram + POL_OFF);
    if (cur != txn->source_policy) {
        reject(txn, "source mismatch at commit");
        return HAEUN_RECFG_REJECTED;
    }
    wr32(vm->ram + POL_OFF, txn->target_policy);
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_AFTER_WRITE)) {
        txn->state = HAEUN_RECFG_ST_FAILED;
        log_msg(txn, "FI after write — entering FAILED; use rollback");
        return HAEUN_RECFG_FAILED;
    }
    txn->state = HAEUN_RECFG_ST_VERIFYING;
    cur = rd32(vm->ram + POL_OFF);
    if (cur != txn->target_policy) {
        txn->state = HAEUN_RECFG_ST_FAILED;
        log_msg(txn, "post-commit verify failed");
        return HAEUN_RECFG_FAILED;
    }
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_AFTER_VERIFY)) {
        txn->state = HAEUN_RECFG_ST_FAILED;
        log_msg(txn, "FI after verify");
        return HAEUN_RECFG_FAILED;
    }
    txn->state = HAEUN_RECFG_ST_COMMITTED;
    log_msg(txn, "committed");
    g_busy = 0;
    return HAEUN_RECFG_OK;
}

/**
 * Restores previous policy ID if a commit write occurred or was requested.
 */
haeun_recfg_result haeun_recfg_rollback(haeun_recfg_txn *txn, haeun_vm *vm) {
    uint32_t cur;
    if (txn == NULL || !ram_ok(vm))
        return HAEUN_RECFG_BAD_ARGUMENT;
    if (txn->state != HAEUN_RECFG_ST_FAILED &&
        txn->state != HAEUN_RECFG_ST_COMMITTING &&
        txn->state != HAEUN_RECFG_ST_VERIFYING) {
        return HAEUN_RECFG_BAD_STATE;
    }
    if (txn->state == HAEUN_RECFG_ST_ROLLED_BACK)
        return HAEUN_RECFG_BAD_STATE;
    txn->state = HAEUN_RECFG_ST_ROLLING_BACK;
    log_msg(txn, "rolling back policy ID only");
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_DURING_ROLLBACK)) {
        txn->state = HAEUN_RECFG_ST_FAILED;
        log_msg(txn, "FI during rollback — FAILED");
        g_busy = 0;
        return HAEUN_RECFG_FAILED;
    }
    wr32(vm->ram + POL_OFF, txn->source_policy);
    cur = rd32(vm->ram + POL_OFF);
    if (cur != txn->source_policy) {
        txn->state = HAEUN_RECFG_ST_FAILED;
        log_msg(txn, "rollback verify failed");
        g_busy = 0;
        return HAEUN_RECFG_FAILED;
    }
    txn->policy_id_restored = 1;
    if (haeun_recfg_fi_hit(HAEUN_RECFG_FI_AFTER_ROLLBACK)) {
        txn->state = HAEUN_RECFG_ST_FAILED;
        log_msg(txn, "FI after rollback");
        g_busy = 0;
        return HAEUN_RECFG_FAILED;
    }
    txn->state = HAEUN_RECFG_ST_ROLLED_BACK;
    log_msg(txn, "rolled back (policy ID scope only)");
    g_busy = 0;
    return HAEUN_RECFG_OK;
}

/**
 * Returns the current transaction state.
 */
haeun_recfg_state haeun_recfg_status(const haeun_recfg_txn *txn) {
    if (txn == NULL)
        return HAEUN_RECFG_ST_FAILED;
    return txn->state;
}
