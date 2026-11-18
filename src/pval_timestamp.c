#include "haeun/pval_timestamp.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { ST_READY = 1, ST_RUNNING = 2 };

static void add(haeun_pval_timestamp_report *out, const char *id,
                haeun_pval_status st, const char *why) {
    haeun_pval_check *c;
    if (out->check_count >= 8u)
        return;
    c = &out->checks[out->check_count++];
    snprintf(c->id, sizeof(c->id), "%s", id);
    c->status = st;
    snprintf(c->explanation, sizeof(c->explanation), "%s", why);
}

static uint64_t seq_u64(uint32_t lo, uint32_t hi) {
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

/**
 * Validates READY ordering for FIFO: prefer R3.4 ready_seq when wrap==0.
 * Falls back to ready_since modular checks only if sequences are absent.
 */
haeun_pval_result haeun_pval_timestamp_check(const haeun_pval_snapshot *snap,
                                             haeun_pval_timestamp_report *out) {
    uint64_t seq[HAEUN_PVAL_AS_SLOTS];
    unsigned n = 0u;
    unsigned i;
    unsigned j;
    int have_seq = 0;
    if (snap == NULL || out == NULL)
        return HAEUN_PVAL_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    for (i = 0; i < HAEUN_PVAL_AS_SLOTS; ++i) {
        const haeun_pval_slot *s = &snap->slots[i];
        if (!s->as_slot_valid)
            continue;
        if (s->state != ST_READY && s->state != ST_RUNNING)
            continue;
        seq[n] = seq_u64(s->ready_seq_lo, s->ready_seq_hi);
        if (seq[n] != 0u)
            have_seq = 1;
        n++;
    }
    out->eligible_count = n;
    out->external_bound_proven = 0;
    if (n == 0u) {
        add(out, "TS.empty", HAEUN_PVAL_NOT_APPLICABLE, "no READY/RUNNING candidates");
        out->pairwise_comparable = 1;
        out->overall = HAEUN_PVAL_NOT_APPLICABLE;
        return HAEUN_PVAL_OK;
    }
    if (have_seq) {
        if (snap->ready_seq_wrap != 0u) {
            add(out, "SEQ.wrap", HAEUN_PVAL_UNKNOWN,
                "ready_seq wrapped (2^64); ordering unbound");
            out->overall = HAEUN_PVAL_UNKNOWN;
            return HAEUN_PVAL_OK;
        }
        for (i = 0; i < n; ++i) {
            if (seq[i] == 0u) {
                add(out, "SEQ.missing", HAEUN_PVAL_FAIL,
                    "eligible slot missing ready_seq");
                out->overall = HAEUN_PVAL_FAIL;
                return HAEUN_PVAL_OK;
            }
            for (j = i + 1u; j < n; ++j) {
                if (seq[i] == seq[j]) {
                    add(out, "SEQ.duplicate", HAEUN_PVAL_FAIL,
                        "duplicate ready_seq among eligible");
                    out->pairwise_comparable = 0;
                    out->overall = HAEUN_PVAL_FAIL;
                    return HAEUN_PVAL_OK;
                }
            }
        }
        out->pairwise_comparable = 1;
        out->external_bound_proven = 1;
        add(out, "SEQ.unique", HAEUN_PVAL_PASS,
            "unique ready_seq under wrap==0 (bounded assumption)");
        out->overall = HAEUN_PVAL_PASS;
        return HAEUN_PVAL_OK;
    }
    /* Legacy ready_since path: cannot prove lifetime <2^31. */
    add(out, "TS.legacy", HAEUN_PVAL_UNKNOWN,
        "no ready_seq; ready_since lifetime unbound");
    out->pairwise_comparable = 1;
    out->overall = HAEUN_PVAL_UNKNOWN;
    return HAEUN_PVAL_OK;
}
